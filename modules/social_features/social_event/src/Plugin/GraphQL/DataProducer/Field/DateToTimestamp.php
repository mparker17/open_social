<?php

namespace Drupal\social_event\Plugin\GraphQL\DataProducer\Field;

use Drupal\datetime\Plugin\Field\FieldType\DateTimeFieldItemList;
use Drupal\datetime\Plugin\Field\FieldType\DateTimeItem;
use Drupal\graphql\Plugin\GraphQL\DataProducer\DataProducerPluginBase;

/**
 * Get the date range as a timestamp.
 *
 * @DataProducer(
 *   id = "date_to_timestamp",
 *   name = @Translation("Date to timestamp"),
 *   description = @Translation("Loads entities from date field."),
 *   produces = @ContextDefinition("int",
 *     label = @Translation("Timestamp")
 *   ),
 *   consumes = {
 *     "field" = @ContextDefinition("any",
 *       label = @Translation("The date field")
 *     )
 *   }
 * )
 */
class DateToTimestamp extends DataProducerPluginBase {

  /**
   * Resolves the request to the requested values.
   *
   * @param \Drupal\datetime\Plugin\Field\FieldType\DateTimeFieldItemList $field
   *   The date field object.
   *
   * @return int|null
   *   A discussion start or end day timestamp.
   */
  public function resolve(DateTimeFieldItemList $field) {
    if (empty($field->getValue())) {
      return NULL;
    }

    return $field->{ DateTimeItem::DATETIME_TYPE_DATE }->getTimestamp();
  }

}
