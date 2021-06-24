<?php

namespace Drupal\social_event\Plugin\GraphQL\DataProducer;

use Drupal\Core\Cache\RefinableCacheableDependencyInterface;
use Drupal\Core\Database\Connection;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\graphql\GraphQL\Buffers\EntityBuffer;
use Drupal\graphql\GraphQL\Buffers\EntityRevisionBuffer;
use Drupal\graphql\GraphQL\Buffers\EntityUuidBuffer;
use Drupal\node\NodeInterface;
use Drupal\social_event\Plugin\GraphQL\QueryHelper\EventManagersQueryHelper;
use Drupal\social_graphql\GraphQL\EntityConnection;
use Drupal\social_graphql\Plugin\GraphQL\DataProducer\Entity\EntityDataProducerPluginBase;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Queries the managers of the event on the platform.
 *
 * @DataProducer(
 *   id = "event_managers",
 *   name = @Translation("Event managers"),
 *   description = @Translation("Loads the managers for a event."),
 *   produces = @ContextDefinition("any",
 *     label = @Translation("EntityConnection")
 *   ),
 *   consumes = {
 *     "event" = @ContextDefinition("entity:node:event",
 *       label = @Translation("Event"),
 *       required = TRUE
 *     ),
 *     "first" = @ContextDefinition("integer",
 *       label = @Translation("First"),
 *       required = FALSE
 *     ),
 *     "after" = @ContextDefinition("string",
 *       label = @Translation("After"),
 *       required = FALSE
 *     ),
 *     "last" = @ContextDefinition("integer",
 *       label = @Translation("Last"),
 *       required = FALSE
 *     ),
 *     "before" = @ContextDefinition("string",
 *       label = @Translation("Before"),
 *       required = FALSE
 *     ),
 *     "reverse" = @ContextDefinition("boolean",
 *       label = @Translation("Reverse"),
 *       required = FALSE,
 *       default_value = FALSE
 *     ),
 *     "sortKey" = @ContextDefinition("string",
 *       label = @Translation("Sort key"),
 *       required = FALSE,
 *       default_value = "CREATED_AT"
 *     )
 *   }
 * )
 */
class EventManagers extends EntityDataProducerPluginBase {

  /**
   * The database service.
   *
   * @var \Drupal\Core\Database\Connection
   */
  protected $database;

  /**
   * {@inheritdoc}
   *
   * @codeCoverageIgnore
   */
  public static function create(ContainerInterface $container, array $configuration, $plugin_id, $plugin_definition) {
    return new static(
      $configuration,
      $plugin_id,
      $plugin_definition,
      $container->get('entity_type.manager'),
      $container->get('graphql.buffer.entity'),
      $container->get('graphql.buffer.entity_uuid'),
      $container->get('graphql.buffer.entity_revision'),
      $container->get('database')
    );
  }

  /**
   * EventManagers constructor.
   *
   * @param array $configuration
   *   The plugin configuration array.
   * @param string $pluginId
   *   The plugin id.
   * @param array $pluginDefinition
   *   The plugin definition array.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entityTypeManager
   *   The entity type manager service.
   * @param \Drupal\graphql\GraphQL\Buffers\EntityBuffer $graphqlEntityBuffer
   *   The GraphQL entity buffer.
   * @param \Drupal\graphql\GraphQL\Buffers\EntityUuidBuffer $graphqlEntityUuidBuffer
   *   The GraphQL entity uuid buffer.
   * @param \Drupal\graphql\GraphQL\Buffers\EntityRevisionBuffer $graphqlEntityRevisionBuffer
   *   The GraphQL entity revision buffer.
   * @param \Drupal\Core\Database\Connection $database
   *   Database Service Object.
   *
   * @codeCoverageIgnore
   */
  public function __construct(
    array $configuration,
    $pluginId,
    array $pluginDefinition,
    EntityTypeManagerInterface $entityTypeManager,
    EntityBuffer $graphqlEntityBuffer,
    EntityUuidBuffer $graphqlEntityUuidBuffer,
    EntityRevisionBuffer $graphqlEntityRevisionBuffer,
    Connection $database
  ) {
    parent::__construct($configuration, $pluginId, $pluginDefinition, $entityTypeManager, $graphqlEntityBuffer, $graphqlEntityUuidBuffer, $graphqlEntityRevisionBuffer);
    $this->database = $database;
  }

  /**
   * Resolves the request to the requested values.
   *
   * @param \Drupal\node\NodeInterface $event
   *   The conversation to fetch participants for.
   * @param int|null $first
   *   Fetch the first X results.
   * @param string|null $after
   *   Cursor to fetch results after.
   * @param int|null $last
   *   Fetch the last X results.
   * @param string|null $before
   *   Cursor to fetch results before.
   * @param bool $reverse
   *   Reverses the order of the data.
   * @param string $sortKey
   *   Key to sort by.
   * @param \Drupal\Core\Cache\RefinableCacheableDependencyInterface $metadata
   *   Cacheability metadata for this request.
   *
   * @return \Drupal\social_graphql\GraphQL\ConnectionInterface
   *   An entity connection with results and data about the paginated results.
   */
  public function resolve(NodeInterface $event, ?int $first, ?string $after, ?int $last, ?string $before, bool $reverse, string $sortKey, RefinableCacheableDependencyInterface $metadata) {
    $query_helper = new EventManagersQueryHelper($sortKey, $this->entityTypeManager, $this->graphqlEntityBuffer, $event, $this->database);
    $metadata->addCacheableDependency($query_helper);

    $connection = new EntityConnection($query_helper);
    $connection->setPagination($first, $after, $last, $before, $reverse);
    return $connection;
  }

}
