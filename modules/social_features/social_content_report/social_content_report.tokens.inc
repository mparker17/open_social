<?php

/**
 * @file
 * Builds placeholder replacement tokens for Social Content Report module.
 */

use Drupal\Core\Render\BubbleableMetadata;
use Drupal\Core\Url;
use Drupal\node\NodeInterface;
use Drupal\social_post\Entity\PostInterface;

/**
 * Implements hook_token_info().
 */
function social_content_report_token_info() {
  $tokens['content_url'] = [
    'name' => t('URL of the reported content.'),
    'description' => t('URL of the content that was reported.'),
  ];

  return [
    'types' => [
      'social_content_report' => [
        'name' => t('Social Content Report'),
        'description' => t('Tokens from the Social Content Report module.'),
      ],
    ],
    'tokens' => [
      'social_content_report' => $tokens,
    ],
  ];
}

/**
 * Implements hook_tokens().
 */
function social_content_report_tokens($type, $tokens, array $data, array $options, BubbleableMetadata $bubbleable_metadata) {
  $replacements = [];

  if ($type === 'social_content_report' && !empty($data['message'])) {
    /** @var \Drupal\message\Entity\Message $message */
    $message = $data['message'];

    $storage = \Drupal::entityTypeManager()->getStorage('flagging');

    foreach ($tokens as $name => $original) {
      // Set the URL for the reported content.
      if ($name === 'content_url' && $message->getFieldValue('field_message_related_object', 'target_type') === 'flagging') {
        // Load the flag.
        $flagging_id = $message->getFieldValue('field_message_related_object', 'target_id');

        /** @var \Drupal\flag\FlaggingInterface $flagging */
        $flagging = $storage->load($flagging_id);

        // Load the flagged entity.
        /** @var \Drupal\Core\Entity\EntityInterface $entity */
        $entity = $flagging->getFlaggable();

        // Set the URL for the entity.
        $replacements[$original] = $entity->toUrl()->setAbsolute()->toString();
      }
    }
  }

  return $replacements;
}

/**
 * Implements hook_tokens_alter().
 */
function social_content_report_tokens_alter(array &$replacements, array $context, BubbleableMetadata $bubbleable_metadata) {
  if ($context['type'] == 'message' && !empty($context['data']['message'])) {
    /** @var Drupal\message\Entity\Message $message */
    $message = $context['data']['message'];

    // Alter the [message:cta_button] or [message:preview] token, and
    // replace it with the rendered content with new text and link.
    if (isset($context['tokens']['cta_button']) || isset($context['tokens']['preview'])) {
      if ($message->hasField('field_message_related_object') && !$message->get('field_message_related_object')->isEmpty()) {
        $target_type = $message->getFieldValue('field_message_related_object', 'target_type');

        if ($target_type === 'flagging') {
          $message_template_id = $message->getTemplate()->id();
          switch ($message_template_id) {
            case 'content_reported':
              // Load the flag.
              $flagging_id = $message->getFieldValue('field_message_related_object', 'target_id');
              $storage = \Drupal::entityTypeManager()->getStorage('flagging');

              // Replace the preview token.
              if (isset($context['tokens']['cta_button'])) {
                $cta_button = [
                  '#theme' => 'message_cta_button',
                  '#link' => Url::fromRoute('view.report_overview.overview')->toString(),
                  '#text' => t('Check the reported contents'),
                ];
                if (!empty($cta_button)) {
                  $replacements[$context['tokens']['cta_button']] = \Drupal::service('renderer')->renderPlain($cta_button);
                }
              }

              // Replace the preview token.
              if (isset($context['tokens']['preview'])) {
                /** @var \Drupal\flag\FlaggingInterface $flagging */
                $flagging = $storage->load($flagging_id);

                // Load the flagged entity.
                $entity = $flagging->getFlaggable();
                $entity_type = $entity->getEntityTypeId();

                $summary = '';
                $preview_info = [];

                switch ($entity_type) {
                  case 'post':
                    if ($entity instanceof PostInterface) {
                      /** @var \Drupal\social_post\Entity\PostInterface $entity */
                      // Get the summary of the comment.
                      if ($entity->hasField('field_post') && !$entity->get('field_post')->isEmpty()) {
                        $summary = _social_comment_get_summary($entity->get('field_post')->getValue());
                      }

                      // Prepare the preview information.
                      $preview_info = [
                        '#theme' => 'message_post_comment_preview',
                        '#summary' => $summary,
                      ];
                    }
                    break;

                  case 'node':
                    if ($entity instanceof NodeInterface) {
                      $date_formatter = \Drupal::service('date.formatter');
                      $date_format = 'social_long_date';

                      /** @var Drupal\node\NodeInterface $entity */
                      $link = Url::fromRoute('entity.node.canonical',
                        ['node' => $entity->id()],
                        ['absolute' => TRUE]
                      )->toString();

                      $author_name = $entity->getOwner()->getDisplayName();
                      $node_type = strtoupper($entity->getType());

                      // Prepare the preview.
                      $preview_info = [
                        '#theme' => 'message_content_preview',
                        '#author_name' => $author_name,
                        '#date' => $date_formatter->format($entity->getCreatedTime(), $date_format),
                        '#title' => $entity->getTitle(),
                        '#type' => $node_type,
                        '#link' => $link,
                      ];
                    }
                    break;

                  case 'comment':
                    /** @var \Drupal\comment\Entity\Comment $entity */
                    if ($entity->hasField('field_comment_body') && !$entity->get('field_comment_body')->isEmpty()) {
                      if ($summary = _social_comment_get_summary($entity->getFieldValue('field_comment_body', 'value'))) {
                        // Prepare the preview information.
                        $preview_info = [
                          '#theme' => 'message_post_comment_preview',
                          '#summary' => $summary,
                        ];
                      }
                    }
                    break;
                }
                $replacements[$context['tokens']['preview']] = \Drupal::service('renderer')
                  ->renderPlain($preview_info);
              }
              break;
          }
        }
      }
    }
  }
}
