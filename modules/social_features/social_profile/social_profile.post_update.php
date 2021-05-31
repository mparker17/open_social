<?php

/**
 * @file
 * Post update functions for Social Profile.
 */

use Drupal\profile\Entity\ProfileInterface;

/**
 * Update Profile names.
 */
function social_profile_post_update_10101_profile_names_update(&$sandbox) {
  /** @var \Drupal\profile\ProfileStorageInterface $profile_storage */
  $profile_storage = \Drupal::entityTypeManager()->getStorage('profile');

  if (!isset($sandbox['count'])) {
    $sandbox['ids'] = \Drupal::entityQuery('profile')
      ->condition('type', 'profile')
      ->accessCheck(FALSE)
      ->execute();
    $sandbox['count'] = count($sandbox['ids']);
  }

  $ids = array_splice($sandbox['ids'], 0, 50);

  // Load profiles by profiles IDs.
  $profiles = $profile_storage->loadMultiple($ids);

  /** @var \Drupal\profile\Entity\ProfileInterface $profile */
  foreach ($profiles as $profile) {
    if ($profile instanceof ProfileInterface) {
      // We need just save the profile. The profile name will be updated by
      // hook "presave".
      // @see social_profile_profile_presave()
      // @see social_profile_privacy_profile_presave()
      $profile->save();
    }
  }

  $sandbox['#finished'] = empty($sandbox['ids']) ? 1 : ($sandbox['count'] - count($sandbox['ids'])) / $sandbox['count'];
}
