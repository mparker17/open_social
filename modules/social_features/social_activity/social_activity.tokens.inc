<?php

/**
 * @file
 * Builds placeholder replacement tokens for Social Activity module.
 */

use Drupal\Core\Render\BubbleableMetadata;
use Drupal\Core\Url;
use Drupal\group\Entity\GroupContentInterface;
use Drupal\node\NodeInterface;

/**
 * Implements hook_token_info().
 */
function social_activity_token_info() {
  // The token additional information has been deprecated in 10.x.
  // It will be removed from 11.x.
  $tokens['additional_information'] = [
    'name' => t('Additional information.'),
    'description' => t('Additional message information. This token is deprecated, please use "preview" and "cta_button" tokens instead in your message templates.'),
  ];

  $tokens['preview'] = [
    'name' => t('Preview.'),
    'description' => t('Preview of the related entity.'),
  ];

  $tokens['cta_button'] = [
    'name' => t('CTA button'),
    'description' => t('A call to action button.'),
  ];

  return [
    'tokens' => [
      'message' => $tokens,
    ],
  ];
}

/**
 * Implements hook_tokens().
 */
function social_activity_tokens($type, $tokens, array $data, array $options, BubbleableMetadata $bubbleable_metadata) {
  $replacements = [];

  if ($type === 'message' && !empty($data['message'])) {
    /** @var \Drupal\message\Entity\Message $message */
    $message = $data['message'];
    if (!empty($message->get('field_message_related_object'))) {

      $target_type = $message->getFieldValue('field_message_related_object', 'target_type');
      $target_id = $message->getFieldValue('field_message_related_object', 'target_id');

      $entity = \Drupal::entityTypeManager()->getStorage($target_type)
        ->load($target_id);
      if ($entity) {
        $date_formatter = \Drupal::service('date.formatter');
        $date_format = 'social_long_date';
        $summary = '';

        if ($target_type === 'post') {
          /** @var \Drupal\social_post\Entity\PostInterface $entity */

          // Get the summary of the comment.
          if ($entity->hasField('field_post') && !$entity->field_post->isEmpty()) {
            $summary = _social_comment_get_summary($entity->field_post->value);
          }

          // Prepare the link to post.
          $link = Url::fromRoute('entity.post.canonical',
            ['post' => $entity->id()],
            ['absolute' => TRUE]
          )->toString();

          // Prepare the preview information.
          $preview_info = [
            '#theme' => 'message_post_comment_preview',
            '#summary' => $summary,
          ];

          // Prepare the CTA button markup.
          $cta_button = [
            '#theme' => 'message_cta_button',
            '#link' => $link,
            '#text' => t('Leave a comment'),
          ];

          // This is for token deprecated token 'additional_information'.
          $additional_information = [
            '#theme' => 'message_post_teaser',
            '#name' => $entity->getOwner()->getDisplayName(),
            '#date' => $date_formatter->format($entity->getCreatedTime(), $date_format),
            '#summary' => $summary,
            '#link' => $link,
          ];
        }
        elseif ($target_type === 'group_content' && $entity instanceof GroupContentInterface) {
          /** @var \Drupal\node\NodeInterface $node */
          $node = $entity->getEntity();

          if ($node instanceof NodeInterface) {
            $link = Url::fromRoute('entity.node.canonical',
              ['node' => $node->id()],
              ['absolute' => TRUE]
            )->toString();

            $author_name = $node->getOwner()->getDisplayName();
            $node_type = strtoupper($node->getType());

            // Prepare the preview.
            $preview_info = [
              '#theme' => 'message_content_preview',
              '#author_name' => $author_name,
              '#date' => $date_formatter->format($node->getCreatedTime(), $date_format),
              '#title' => $node->getTitle(),
              '#type' => $node_type,
              '#link' => $link,
            ];

            // Prepare the CTA button markup.
            $cta_button = [
              '#theme' => 'message_cta_button',
              '#link' => $link,
              '#text' => t('Read more'),
            ];

            // This is for token deprecated token 'additional_information'.
            $additional_information = [
              '#theme' => 'message_node_teaser',
              '#link' => $link,
              '#type' => $node_type,
            ];
          }
        }

        // Replace tokens.
        foreach ($tokens as $name => $original) {
          switch ($name) {
            // The token 'additional_information'̊ is deprecated and
            // will be removed in 11.x release of Open Social.
            case 'additional_information':
              if (!empty($additional_information)) {
                $replacements[$original] = \Drupal::service('renderer')->renderPlain($additional_information);
              }
              break;

            case 'preview':
              if (!empty($preview_info)) {
                $replacements[$original] = \Drupal::service('renderer')->renderPlain($preview_info);
              }
              break;

            case 'cta_button':
              if (!empty($cta_button)) {
                $replacements[$original] = \Drupal::service('renderer')->renderPlain($cta_button);
              }
              break;
          }
        }
      }
    }
  }

  return $replacements;
}
