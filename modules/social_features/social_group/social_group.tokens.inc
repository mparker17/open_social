<?php

/**
 * @file
 * Builds placeholder replacement tokens for Social Group module.
 */

use Drupal\file\Entity\File;
use Drupal\group\Entity\Group;
use Drupal\image\Entity\ImageStyle;
use Drupal\user\UserInterface;
use Drupal\Core\Render\BubbleableMetadata;
use Drupal\Core\Url;
use Drupal\Core\Render\Markup;

/**
 * Implements hook_token_info().
 */
function social_group_token_info() {

  $type = [
    'name' => t('Social Group'),
    'description' => t('Tokens from the social group module.'),
  ];

  $social_group['content_type'] = [
    'name' => t('The group content type.'),
    'description' => t('The type of the content that is created in the group.'),
  ];

  $social_group['content_url'] = [
    'name' => t('The group content url.'),
    'description' => t('The url to the content that is created in the group.'),
  ];

  $social_group['created_entity_link_html'] = [
    'name' => t('The (html) link to the created entity.'),
    'description' => t('The link to the created entity, can be post or node, will include raw HTML.'),
  ];

  return [
    'types' => ['social_group' => $type],
    'tokens' => [
      'social_group' => $social_group,
    ],
  ];
}

/**
 * Implements hook_tokens().
 */
function social_group_tokens($type, $tokens, array $data, array $options, BubbleableMetadata $bubbleable_metadata) {
  $replacements = [];

  if ($type == 'social_group' && !empty($data['message'])) {
    /** @var \Drupal\message\Entity\Message $message */
    $message = $data['message'];

    foreach ($tokens as $name => $original) {
      switch ($name) {

        case 'content_type':
        case 'content_url':
        case 'created_entity_link_html':

          // Get the related entity.
          if (!empty($message->get('field_message_related_object'))) {
            $target_type = $message->getFieldValue('field_message_related_object', 'target_type');
            $target_id = $message->getFieldValue('field_message_related_object', 'target_id');
            $entity = \Drupal::entityTypeManager()
              ->getStorage($target_type)
              ->load($target_id);

            if (is_object($entity)) {
              switch ($target_type) {

                // If it's group content.
                case 'group_content':
                  /** @var \Drupal\group\Entity\GroupContent $entity */
                  $group_content_type = $entity->getGroupContentType();
                  if (!empty($group_content_type)) {
                    $display_name = $group_content_type->label();
                    $content_url = Url::fromRoute('entity.node.canonical',
                      ['node' => $entity->getEntity()->id()],
                      ['absolute' => TRUE]
                    );
                  }
                  break;

                // If it's node.
                case 'node':
                  $display_name = $entity->bundle();
                  break;

                // When it's a post or photo post.
                case 'photo':
                case 'post':
                  $display_name = mb_strtolower($entity->getEntityType()->getLabel());
                  $content_url = Url::fromRoute('entity.post.canonical',
                    ['post' => $entity->id()],
                    ['absolute' => TRUE]
                  );
                  break;

              }

              // When a name of content name starts from a vowel letter then
              // will be added "an" before this name. For example "an
              // event".
              if (isset($display_name)) {
                if (preg_match('/^[aeiou]/', $display_name)) {
                  $display_name = t('an @content_type', ['@content_type' => $display_name]);
                }
                else {
                  $display_name = t('a @content_type', ['@content_type' => $display_name]);
                }
              }

              if ($name === 'content_url') {
                if (isset($content_url)) {
                  $replacements[$original] = $content_url->toString();
                }
              }
              elseif ($name === 'content_type') {
                if (isset($display_name)) {
                  $replacements[$original] = $display_name;
                }
              }
              elseif ($name === 'created_entity_link_html') {
                // We should only use the label of entities who have a label.
                if ($link_label = $entity->label()) {
                  $entity_link_html = $display_name . ' <a href="' . $content_url->toString() . '">' . $link_label . '</a>';
                }
                else {
                  $entity_link_html = '<a href="' . $content_url->toString() . '">' . $display_name . '</a>';
                }

                $replacements[$original] = Markup::create($entity_link_html);
              }

            }
          }

          break;
      }
    }
  }

  return $replacements;
}

/**
 * Implements hook_tokens_alter().
 */
function social_group_tokens_alter(array &$replacements, array $context, BubbleableMetadata $bubbleable_metadata) {
  if ($context['type'] == 'message' && !empty($context['data']['message'])) {
    // Alter the [message:cta_button] and [message:preview] token, and
    // replace it with the rendered content with new text and link.
    if (isset($context['tokens']['cta_button'])|| isset($context['tokens']['preview'])) {
      /** @var Drupal\message\Entity\Message $message */
      $message = $context['data']['message'];
      if (isset($message->field_message_related_object)) {
        $target_type = $message->getFieldValue('field_message_related_object', 'target_type');
        $target_id = $message->getFieldValue('field_message_related_object', 'target_id');

        if (!empty($target_type) && $target_type === 'group_content') {
          /** @var \Drupal\group\Entity\GroupContentInterface $group_content */
          $group_content = \Drupal::entityTypeManager()
            ->getStorage($target_type)
            ->load($target_id);

          if (!empty($group_content)) {
            $type = $group_content->getContentPlugin()->getPluginId();
            if ($type === 'group_membership') {
              $message_template_id = $message->getTemplate()->id();
              $link = $group_content->getGroup()->toUrl()->toString();
              switch ($message_template_id) {
                case 'approve_request_join_group':
                  if (isset($context['tokens']['cta_button'])) {
                    $cta_button = [
                      '#theme' => 'message_cta_button',
                      '#link' => $link,
                      '#text' => t('Explore the group'),
                    ];
                    $replacements[$context['tokens']['cta_button']] = \Drupal::service('renderer')
                      ->renderPlain($cta_button);
                  }
                  // Replace the preview token.
                  if (isset($context['tokens']['preview'])) {
                    // Get the group entity.
                    $group = $group_content->getGroup();
                    if ($group instanceof Group) {
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

                case 'join_to_group':
                  if (isset($context['tokens']['cta_button'])) {
                    $cta_button = [
                      '#theme' => 'message_cta_button',
                      '#link' => Url::fromRoute('view.group_members.page_group_members', ['group' => $group_content->getGroup()->id()])->toString(),
                      '#text' => t('See all members'),
                    ];
                    $replacements[$context['tokens']['cta_button']] = \Drupal::service('renderer')
                      ->renderPlain($cta_button);
                  }
                  // Replace the preview token.
                  if (isset($context['tokens']['preview'])) {
                    /** @var \Drupal\user\Entity\User $user */
                    $user = $group_content->getEntity();
                    if ($user instanceof UserInterface) {
                      $entity_manager = \Drupal::entityTypeManager();
                      /** @var \Drupal\profile\ProfileStorageInterface $profile_storage */
                      $profile_storage = $entity_manager->getStorage('profile');
                      /** @var \Drupal\profile\Entity\Profile $profile */
                      $profile = $profile_storage->loadByUser($user, 'profile');
                      // Add the profile image.
                      /** @var Drupal\image\Entity\ImageStyle $image_style */
                      $image_style = ImageStyle::load('social_medium');
                      if (!empty($profile->field_profile_image->entity)) {
                        $image_url = $image_style->buildUrl($profile->field_profile_image->entity->getFileUri());
                      }
                      elseif ($default_image = social_profile_get_default_image()) {
                        // Add default image.
                        if (!empty($default_image['id'])) {
                          $file = File::load($default_image['id']);
                          $image_url = $image_style->buildUrl($file->getFileUri());
                        }
                      }
                      // Add the profile image.
                      $preview_info = [
                        '#theme' => 'message_user_profile_preview',
                        '#profile_name' => $user->getDisplayName(),
                        '#profile_home' => Url::fromRoute('entity.user.canonical', ['user' => $user->id()]),
                        '#profile_image' => $image_url ?? NULL,
                        '#profile_function' => $profile->getFieldValue('field_profile_function', 'value'),
                        '#profile_organization' => $profile->getFieldValue('field_profile_organization', 'value'),
                      ];
                      $replacements[$context['tokens']['preview']] = \Drupal::service('renderer')
                        ->renderPlain($preview_info);
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
