<?php

/**
 * @file
 * Builds placeholder replacement tokens for Social Group Request module.
 */

use Drupal\Core\Render\BubbleableMetadata;
use Drupal\Core\Url;
use Drupal\group\Entity\Group;

/**
 * Implements hook_token_info().
 */
function social_group_request_token_info() {

  $type = [
    'name' => t('Social Group Request'),
    'description' => t('Tokens from the social group request module.'),
  ];

  $social_group_request['request_page_link'] = [
    'name' => t('Request page link.'),
    'description' => t('The link to the request page of the group.'),
  ];

  return [
    'types' => ['social_group_request' => $type],
    'tokens' => [
      'social_group_request' => $social_group_request,
    ],
  ];
}

/**
 * Implements hook_tokens().
 */
function social_group_request_tokens($type, $tokens, array $data, array $options, BubbleableMetadata $bubbleable_metadata) {
  $replacements = [];

  if ($type === 'social_group_request' && !empty($data['message'])) {
    /** @var Drupal\message\Entity\Message $message */
    $message = $data['message'];

    foreach ($tokens as $name => $original) {
      if ($name === 'request_page_link') {
        // Get the related entity.
        if ($message->hasField('field_message_related_object') && !$message->get('field_message_related_object')->isEmpty()) {
          $target_type = $message->getFieldValue('field_message_related_object', 'target_type');
          $target_id = $message->getFieldValue('field_message_related_object', 'target_id');

          if (isset($target_type) && !empty($target_type) && $target_type === 'group_content') {
            /** @var \Drupal\group\Entity\GroupContentInterface $group_content */
            $group_content = \Drupal::entityTypeManager()
              ->getStorage($target_type)
              ->load($target_id);

            if (!empty($group_content)) {
              $replacements[$original] = Url::fromRoute('view.group_pending_members.membership_requests', ['arg_0' => $group_content->getGroup()->id()])->toString();
            }
          }
        }
      }
    }
  }

  return $replacements;
}

/**
 * Implements hook_tokens_alter().
 */
function social_group_request_tokens_alter(array &$replacements, array $context, BubbleableMetadata $bubbleable_metadata) {
  if ($context['type'] == 'message' && !empty($context['data']['message'])) {
    /** @var Drupal\message\Entity\Message $message */
    $message = $context['data']['message'];

    // Alter the [message:cta_button] and [message:preview] token, and
    // replace it with the rendered content with new text and link.
    if (isset($context['tokens']['cta_button'])|| isset($context['tokens']['preview'])) {
      if ($message->hasField('field_message_related_object') && !$message->get('field_message_related_object')->isEmpty()) {
        $target_type = $message->getFieldValue('field_message_related_object', 'target_type');
        $target_id = $message->getFieldValue('field_message_related_object', 'target_id');

        if (isset($target_type) && !empty($target_type) && $target_type === 'group_content') {
          /** @var \Drupal\group\Entity\GroupContentInterface $group_content */
          $group_content = \Drupal::entityTypeManager()
            ->getStorage($target_type)
            ->load($target_id);

          if (!empty($group_content)) {
            // Get the group entity.
            $group = $group_content->getGroup();
            if ($group instanceof Group) {
              $link = Url::fromRoute('view.group_pending_members.membership_requests', ['arg_0' => $group->id()],
                ['absolute' => TRUE])->toString();
              $message_template_id = $message->getTemplate()->id();
              switch ($message_template_id) {
                case 'request_to_join_a_group':
                  if (isset($context['tokens']['cta_button'])) {
                    $cta_button = [
                      '#theme' => 'message_cta_button',
                      '#link' => $link,
                      '#text' => t('View the requests'),
                    ];
                    $replacements[$context['tokens']['cta_button']] = \Drupal::service('renderer')->renderPlain($cta_button);
                    // Replace the preview token.
                    if (isset($context['tokens']['preview'])) {
                      /** @var \Drupal\social_group\GroupStatistics $group_statistics */
                      $group_statistics = \Drupal::service('social_group.group_statistics');
                      // Add the group preview.
                      $preview_info = [
                        '#theme' => 'message_group_preview',
                        '#group_title' => $group->label(),
                        '#group_type' => strtoupper($group->getGroupType()->label()),
                        '#group_members' => $group_statistics->getGroupMemberCount($group),
                      ];
                      $replacements[$context['tokens']['preview']] = \Drupal::service('renderer')->renderPlain($preview_info);
                    }
                  }
                  break;
              }
            }
          }
        }
      }
    }
  }
}
