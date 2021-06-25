<?php

namespace Drupal\social_event\Plugin\GraphQL\QueryHelper;

use Drupal\Core\Database\Connection;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Entity\Query\QueryInterface;
use Drupal\graphql\GraphQL\Buffers\EntityBuffer;
use Drupal\node\NodeInterface;
use Drupal\social_graphql\GraphQL\ConnectionQueryHelperBase;
use Drupal\social_graphql\Wrappers\Cursor;
use Drupal\social_graphql\Wrappers\Edge;
use Drupal\user\UserInterface;
use GraphQL\Deferred;
use GraphQL\Executor\Promise\Adapter\SyncPromise;

/**
 * Loads event managers.
 */
class EventManagersQueryHelper extends ConnectionQueryHelperBase {

  /**
   * The event for which managers are being fetched.
   *
   * @var \Drupal\node\NodeInterface
   */
  protected NodeInterface $event;

  /**
   * The database service.
   *
   * @var \Drupal\Core\Database\Connection
   */
  protected Connection $database;

  /**
   * EventManagersQueryHelper constructor.
   *
   * @param string $sort_key
   *   The key that is used for sorting.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   The Drupal entity type manager.
   * @param \Drupal\graphql\GraphQL\Buffers\EntityBuffer $graphql_entity_buffer
   *   The GraphQL entity buffer.
   * @param \Drupal\node\NodeInterface $event
   *   The event.
   * @param \Drupal\Core\Database\Connection $database
   *   The database.
   */
  public function __construct(
    string $sort_key,
    EntityTypeManagerInterface $entity_type_manager,
    EntityBuffer $graphql_entity_buffer,
    NodeInterface $event,
    Connection $database
  ) {
    parent::__construct($sort_key, $entity_type_manager, $graphql_entity_buffer);

    $this->event = $event;
    $this->database = $database;
  }

  /**
   * {@inheritdoc}
   */
  public function getQuery() : QueryInterface {
    $query = $this->database->select('node__field_event_managers', 'fem');
    $query->addField('fem', 'field_event_managers_target_id');
    $query->condition('entity_id', $this->event->id());
    $query->condition('revision_id', $this->event->getLoadedRevisionId());
    $uids = $query->execute()->fetchCol();

    return $this->entityTypeManager->getStorage('user')
      ->getQuery()
      ->accessCheck(TRUE)
      ->condition('uid', $uids ?: NULL, 'IN');
  }

  /**
   * {@inheritdoc}
   */
  public function getCursorObject(string $cursor) : ?Cursor {
    $cursor_object = Cursor::fromCursorString($cursor);

    return !is_null($cursor_object) && $cursor_object->isValidFor($this->sortKey, 'node')
      ? $cursor_object
      : NULL;
  }

  /**
   * {@inheritdoc}
   */
  public function getIdField() : string {
    return 'uid';
  }

  /**
   * {@inheritdoc}
   */
  public function getSortField() : string {
    switch ($this->sortKey) {
      case 'CREATED_AT':
        return 'created';

      default:
        throw new \InvalidArgumentException("Unsupported sortKey for sorting '{$this->sortKey}'");
    }
  }

  /**
   * {@inheritdoc}
   */
  public function getAggregateSortFunction() : ?string {
    return NULL;
  }

  /**
   * {@inheritdoc}
   */
  public function getLoaderPromise(array $result) : SyncPromise {
    // In case of no results we create a callback the returns an empty array.
    if (empty($result)) {
      $callback = static fn () => [];
    }
    // Otherwise we create a callback that uses the GraphQL entity buffer to
    // ensure the entities for this query are only loaded once. Even if the
    // results are used multiple times.
    else {
      $buffer = \Drupal::service('graphql.buffer.entity');
      $callback = $buffer->add('user', array_values($result));
    }

    return new Deferred(
      function () use ($callback) {
        return array_map(
          fn (UserInterface $entity) => new Edge(
            $entity,
            new Cursor('user', $entity->id(), $this->sortKey, $this->getSortValue($entity))
          ),
          $callback()
        );
      }
    );
  }

  /**
   * Get the value for an entity based on the sort key for this connection.
   *
   * @param \Drupal\user\UserInterface $user
   *   The moderator entity for the user in this conversation.
   *
   * @return mixed
   *   The sort value.
   */
  protected function getSortValue(UserInterface $user) {
    switch ($this->sortKey) {
      case 'CREATED_AT':
        return $user->getCreatedTime();

      default:
        throw new \InvalidArgumentException("Unsupported sortKey for pagination '{$this->sortKey}'");
    }
  }

}
