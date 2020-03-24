<?php

namespace Drupal\social_group_invite\Plugin\Block;

use Drupal\Core\Access\AccessResult;
use Drupal\Core\Block\BlockBase;
use Drupal\Core\Plugin\ContainerFactoryPluginInterface;
use Drupal\Core\Routing\RouteMatchInterface;
use Drupal\Core\Session\AccountInterface;
use Drupal\Core\Url;
use Drupal\group\Entity\GroupInterface;
use Drupal\node\Entity\Node;
use Drupal\node\NodeInterface;
use Drupal\ginvite\GroupInvitationLoaderInterface;
use Drupal\social_group\Entity\Group;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Provides a 'SocialInviteLocalActionsBlock' block.
 *
 * @Block(
 *  id = "social_invite_actions_block",
 *  admin_label = @Translation("Social Invite Actions block"),
 * )
 */
class SocialInviteLocalActionsBlock extends BlockBase implements ContainerFactoryPluginInterface {

  /**
   * The route match.
   *
   * @var \Drupal\Core\Routing\RouteMatchInterface
   */
  protected $routeMatch;

  /**
   * The route match.
   *
   * @var \Drupal\ginvite\GroupInvitationLoaderInterface
   */
  protected $inviteService;

  /**
   * EventAddBlock constructor.
   *
   * @param array $configuration
   *   The given configuration.
   * @param string $plugin_id
   *   The given plugin id.
   * @param mixed $plugin_definition
   *   The given plugin definition.
   * @param \Drupal\Core\Routing\RouteMatchInterface $routeMatch
   *   The route match.
   * @param \Drupal\ginvite\GroupInvitationLoaderInterface $inviteService
   *   The tag service.
   */
  public function __construct(array $configuration, $plugin_id, $plugin_definition, RouteMatchInterface $routeMatch, GroupInvitationLoaderInterface $inviteService) {
    parent::__construct($configuration, $plugin_id, $plugin_definition);
    $this->routeMatch = $routeMatch;
    $this->inviteService = $inviteService;
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container, array $configuration, $plugin_id, $plugin_definition) {
    return new static(
      $configuration,
      $plugin_id,
      $plugin_definition,
      $container->get('current_route_match'),
      $container->get('ginvite.invitation_loader')
    );
  }

  /**
   * {@inheritdoc}.
   */
  protected function blockAccess(AccountInterface $account) {
    $group = _social_group_get_current_group();
    if ($group instanceof GroupInterface) {
      // If group allows Group Invites by content plugin.
      $group_type = $group->getGroupType();
      if (!$group_type->hasContentPlugin('group_invitation')) {
        return AccessResult::forbidden();
      }
      // Only when user has correct access.
      if ($group->hasPermission('invite users to group', $account)) {
        return AccessResult::allowed();
      }
    }

    return AccessResult::forbidden();
  }

  /**
   * {@inheritdoc}
   */
  public function getCacheContexts() {
    $cache_contexts = parent::getCacheContexts();
    $cache_contexts[] = 'user.group_permissions';
    $cache_contexts[] = 'group';
    $cache_contexts[] = 'route.group';
    return $cache_contexts;
  }

  /**
   * {@inheritdoc}
   */
  public function getCacheTags() {
    $cache_tags = parent::getCacheTags();
    $group = $this->routeMatch->getParameter('group');

    if ($group instanceof GroupInterface) {
      $cache_tags[] = 'group:' . $group->id();
      $cache_tags[] = 'group_content_type_list';
    }

    return $cache_tags;
  }

  /**
   * {@inheritdoc}
   */
  public function build() {
    $build = [];

    // Get current group so we can build correct links.
    $group = _social_group_get_current_group();
    $group_type = $group->getGroupType();
    if ($group instanceof GroupInterface && $group_type->hasContentPlugin('group_invitation')) {
      $links = [
        '#type' => 'dropbutton',
        '#links' => [
          'title' => [
            'title' => $this->t('Add members'),
            'url' => Url::fromRoute('<current>', []),
          ],
          'add_directly' => [
            'title' => $this->t('Add directly'),
            'url' => Url::fromRoute('entity.group_content.add_form', ['plugin_id' => 'group_membership', 'group' => $group->id()]),
          ],
          'invite_by_mail' => [
            'title' => $this->t('Invite by mail'),
            'url' => Url::fromRoute('ginvite.invitation.bulk', ['group' => $group->id()]),
          ],
          'view_invites' => [
            'title' => $this->t('View invites'),
            'url' => Url::fromRoute('view.group_invitations.page_1', ['group' => $group->id()]),
          ],
        ],
      ];

      $build['content'] = $links;
    }

    return $build;
  }

}
