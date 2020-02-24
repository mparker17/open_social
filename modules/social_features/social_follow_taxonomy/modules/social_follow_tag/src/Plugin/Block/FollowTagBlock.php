<?php

namespace Drupal\social_follow_tag\Plugin\Block;

use Drupal\Core\Block\BlockBase;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBuilderInterface;
use Drupal\Core\Plugin\ContainerFactoryPluginInterface;
use Drupal\Core\Render\RendererInterface;
use Drupal\flag\Entity\Flag;
use Drupal\flag\FlagInterface;
use Drupal\flag\FlagServiceInterface;
use Drupal\social_tagging\SocialTaggingService;
use Drupal\taxonomy\Entity\Term;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Provides a 'FollowTagBlock' block.
 *
 * @Block(
 *  id = "follow_tag_block",
 *  admin_label = @Translation("Follow tag"),
 * )
 */
class FollowTagBlock extends BlockBase implements ContainerFactoryPluginInterface {

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected $entityTypeManager;

  /**
   * The form builder.
   *
   * @var \Drupal\Core\Form\FormBuilderInterface
   */
  protected $formBuilder;

  /**
   * The route match.
   *
   * @var \Drupal\social_tagging\SocialTaggingService
   */
  protected $tagService;

  /**
   * Flag service.
   *
   * @var \Drupal\flag\FlagServiceInterface
   */
  protected $flagService;

  /**
   * The renderer service.
   *
   * @var \Drupal\Core\Render\RendererInterface
   */
  protected $renderer;

  /**
   * SearchHeroBlock constructor.
   *
   * @param array $configuration
   *   The given configuration.
   * @param string $plugin_id
   *   The plugin id.
   * @param mixed $plugin_definition
   *   The plugin definition.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   The entity type manager.
   * @param \Drupal\Core\Form\FormBuilderInterface $formBuilder
   *   The form builder.
   * @param \Drupal\social_tagging\SocialTaggingService $tagging_service
   *   The tag service.
   * @param \Drupal\flag\FlagServiceInterface $flag_service
   *   Flag service.
   * @param \Drupal\Core\Render\RendererInterface $renderer
   *   The renderer.
   */
  public function __construct(
    array $configuration,
    $plugin_id,
    $plugin_definition,
    EntityTypeManagerInterface $entity_type_manager,
    FormBuilderInterface $formBuilder,
    SocialTaggingService $tagging_service,
    FlagServiceInterface $flag_service,
    RendererInterface $renderer
  ) {
    parent::__construct($configuration, $plugin_id, $plugin_definition);
    $this->entityTypeManager = $entity_type_manager;
    $this->formBuilder = $formBuilder;
    $this->tagService = $tagging_service;
    $this->flagService = $flag_service;
    $this->renderer = $renderer;
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container, array $configuration, $plugin_id, $plugin_definition) {
    return new static(
      $configuration,
      $plugin_id,
      $plugin_definition,
      $container->get('entity_type.manager'),
      $container->get('form_builder'),
      $container->get('social_tagging.tag_service'),
      $container->get('flag'),
      $container->get('renderer')
    );
  }

  /**
   * {@inheritdoc}
   */
  public function build() {
    $build = [];
    $terms = [];

    if ($this->tagService->allowSplit()) {
      foreach ($this->tagService->getCategories() as $tid => $value) {
        if (!empty($this->tagService->getChildren($tid))) {
          $identifiers[] = social_tagging_to_machine_name($value);
        }
      }
    }

    foreach ($identifiers as $identifier) {
      if (isset($_GET[$identifier])) {
        $query = $_GET[$identifier];
        $terms[] = $query;
      }
    }

    $flag_link_service = \Drupal::service('flag.link_builder');

    // Just a simple way to add all tags to the array.
    $tags = [];
    foreach ($terms as $term_ids) {
      foreach ($term_ids as $term_key => $term) {

        $taxonomy_term = Term::load($term);

        $nodes = $this->entityTypeManager
          ->getStorage('node')
          ->loadByProperties(['social_tagging' => $term]);
        $related_content = [];
        foreach ($nodes as $node) {
          $related_content[$node->bundle()]['label'] = $node->type->entity->label();
          if ($related_content[$node->bundle()]) {
            if (isset($related_content[$node->bundle()]['count'])) {
              $related_content[$node->bundle()]['count'] += 1;
            }
            else {
              $related_content[$node->bundle()]['count'] = 1;
            }

            $related_content[$node->bundle()]['nid'][] = $node->id();
          }
        }

        $flag_link = $flag_link_service->build($taxonomy_term->getEntityTypeId(), $taxonomy_term->id(), 'follow_term');

        $follow = FALSE;
        $flag = Flag::load('follow_term');
        if ($flag instanceof FlagInterface) {
          if (!empty($this->flagService->getFlagging($flag, $taxonomy_term, \Drupal::currentUser()))) {
            $follow = TRUE;
          }
        }

        if ($follow) {
          $tags[$term] = [
            'name' => $taxonomy_term->getName(),
            'flag' => $flag_link,
            'related_content' => $related_content,
          ];
        }
      }
    }

    $renderable = [
      '#theme' => 'search_follow_tag',
      '#tagstitle' => t('Tags'),
      '#tags' => $tags,
    ];

    $build['content']['#markup'] = $this->renderer->render($renderable);

    return $build;
  }

  /**
   * {@inheritdoc}
   */
  public function getCacheMaxAge() {
    return 0;
  }

}
