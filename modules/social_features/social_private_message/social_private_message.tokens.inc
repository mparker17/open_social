<?php

/**
 * @file
 * Builds placeholder replacement tokens for message-related data.
 */

use Drupal\Core\Render\BubbleableMetadata;
use Drupal\Core\Url;

/**
 * Implements hook_tokens_alter().
 */
function social_private_message_tokens_alter(array &$replacements, array $context, BubbleableMetadata $bubbleable_metadata) {
  if ($context['type'] == 'message' && !empty($context['data']['message'])) {
    /** @var Drupal\message\Entity\Message $message */
    $message = $context['data']['message'];

    // Alter the [message:cta_button] or [message:preview] token, and
    // replace it with the rendered content with new text and link.
    if (isset($context['tokens']['cta_button']) || isset($context['tokens']['preview'])) {
      if ($message->hasField('field_message_related_object') && !$message->get('field_message_related_object')->isEmpty()) {
        $target_type = $message->getFieldValue('field_message_related_object', 'target_type');
        $target_id = $message->getFieldValue('field_message_related_object', 'target_id');

        if (isset($target_type) && !empty($target_type)) {
          $message_template_id = $message->getTemplate()->id();
          switch ($message_template_id) {
            case 'create_private_message':
              /** @var \Drupal\private_message\Entity\PrivateMessage $entity */
              $entity = \Drupal::entityTypeManager()
                ->getStorage($target_type)
                ->load($target_id);

              // Replace the preview token.
              if (isset($context['tokens']['cta_button'])) {
                // Use the mapper service to get the ID of the thread.
                $pmService = \Drupal::service('private_message.mapper');
                $thread_id = $pmService->getThreadIdFromMessage($entity);
                // Build the url.
                $thread_url = Url::fromRoute('entity.private_message_thread.canonical',
                  ['private_message_thread' => $thread_id],
                  ['absolute' => TRUE]
                );
                $cta_button = [
                  '#theme' => 'message_cta_button',
                  '#link' => $thread_url->toString(),
                  '#text' => t('Reply to this message'),
                ];
                if (!empty($cta_button)) {
                  $replacements[$context['tokens']['cta_button']] = \Drupal::service('renderer')->renderPlain($cta_button);
                }
              }

              // Replace the preview token.
              if (isset($context['tokens']['preview'])) {
                // Prepare the preview.
                $preview_info = [
                  '#theme' => 'message_post_comment_preview',
                  '#summary' => $entity->getMessage(),
                ];

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
