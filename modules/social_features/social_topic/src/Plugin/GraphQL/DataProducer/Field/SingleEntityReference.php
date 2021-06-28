<?php

namespace Drupal\social_topic\Plugin\GraphQL\DataProducer\Field;

use Drupal\graphql\Plugin\GraphQL\DataProducer\DataProducerPluginBase;

/**
 * Keep first entity from an array of entities.
 *
 * @DataProducer(
 *   id = "single_entity",
 *   name = @Translation("Single entity"),
 *   description = @Translation("Keep first entity from an array of entities."),
 *   produces = @ContextDefinition("entity",
 *     label = @Translation("Entity"),
 *     multiple = TRUE
 *   ),
 *   consumes = {
 *     "entities" = @ContextDefinition("any",
 *       label = @Translation("Parent entity")
 *     )
 *   }
 * )
 */
class SingleEntityReference extends DataProducerPluginBase {

  /**
   * Resolve given entities.
   *
   * @param array $entities
   *
   * @return \GraphQL\Deferred|null
   */
  public function resolve(array $entities) {
    if (!is_array($entities)) {
      return NULL;
    }

    return reset($entities);
  }

}
