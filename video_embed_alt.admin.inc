<?php

/**
 * @file
 * Settings page for Video Embed Alt.
 */

/**
 * Form constructor for the provider settings.
 *
 * Each value here either goes into every player's iframe src or is a server
 * the site sends requests to, which is why saving is strict about scheme and
 * host rather than trusting the administrator's typing.
 *
 * @see video_embed_alt_settings_form_validate()
 * @see video_embed_alt_settings_form_submit()
 */
function video_embed_alt_settings_form($form, &$form_state) {
  $settings = video_embed_alt_settings();

  $form['intro'] = array(
    '#markup' => '<p>' . t('Only the video id is stored with each piece of content. Players are built from these addresses when the page is shown, so if a site moves, changing the address here updates every existing video. <code>{id}</code> marks where the video id goes.') . '</p>',
  );

  $form['rumble'] = array(
    '#type' => 'fieldset',
    '#title' => t('Rumble'),
  );
  $form['rumble']['rumble_domains'] = array(
    '#type' => 'textarea',
    '#title' => t('Accepted domains'),
    '#default_value' => implode("\n", $settings['rumble_domains']),
    '#rows' => 3,
    '#description' => t('One per line, without "https://" or "www.". Links pasted by editors must be on one of these. Keep an old domain listed after a move: existing content still points at it.'),
    '#required' => TRUE,
  );
  $form['rumble']['rumble_embed_template'] = array(
    '#type' => 'textfield',
    '#title' => t('Embed address'),
    '#default_value' => $settings['rumble_embed_template'],
    '#required' => TRUE,
  );
  $form['rumble']['rumble_oembed_endpoint'] = array(
    '#type' => 'textfield',
    '#title' => t('oEmbed lookup address'),
    '#default_value' => $settings['rumble_oembed_endpoint'],
    '#description' => t('Rumble page links carry a different id from the one the player needs. This lookup finds it, and fetches the title and thumbnail.'),
    '#required' => TRUE,
  );

  $form['bitchute'] = array(
    '#type' => 'fieldset',
    '#title' => t('BitChute'),
  );
  $form['bitchute']['bitchute_domains'] = array(
    '#type' => 'textarea',
    '#title' => t('Accepted domains'),
    '#default_value' => implode("\n", $settings['bitchute_domains']),
    '#rows' => 3,
    '#description' => t('One per line, without "https://" or "www.".'),
    '#required' => TRUE,
  );
  $form['bitchute']['bitchute_embed_template'] = array(
    '#type' => 'textfield',
    '#title' => t('Embed address'),
    '#default_value' => $settings['bitchute_embed_template'],
    '#description' => t('Use www.bitchute.com: old.bitchute.com redirects UK visitors away from embeds.'),
    '#required' => TRUE,
  );
  $form['bitchute']['bitchute_video_template'] = array(
    '#type' => 'textfield',
    '#title' => t('Video page address'),
    '#default_value' => $settings['bitchute_video_template'],
    '#description' => t('Stored with the content, and used for the "Watch on BitChute" link.'),
    '#required' => TRUE,
  );
  $form['bitchute']['bitchute_api_endpoint'] = array(
    '#type' => 'textfield',
    '#title' => t('Video details lookup address'),
    '#default_value' => $settings['bitchute_api_endpoint'],
    '#description' => t('Fetches the title and thumbnail. BitChute does not document this service, so it may change without notice; when it fails, videos still embed and editors are asked to upload an image.'),
    '#required' => TRUE,
  );

  $form['actions']['#type'] = 'actions';
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
  );
  return $form;
}

/**
 * Form validation handler for video_embed_alt_settings_form().
 */
function video_embed_alt_settings_form_validate($form, &$form_state) {
  $values = &$form_state['values'];

  foreach (video_embed_alt_providers() as $name => $title) {
    $domains = video_embed_alt_parse_domains($values[$name . '_domains']);
    if (!$domains) {
      form_set_error($name . '_domains', t('@provider: enter at least one domain.', array('@provider' => $title)));
      continue;
    }
    foreach ($domains as $domain) {
      if (!preg_match('/^[a-z0-9]([a-z0-9-]*[a-z0-9])?(\.[a-z0-9]([a-z0-9-]*[a-z0-9])?)+$/', $domain)) {
        form_set_error($name . '_domains', t('@provider: %domain is not a domain name.', array('@provider' => $title, '%domain' => $domain)));
        continue 2;
      }
    }
    $values[$name . '_domains'] = $domains;

    // Templates end up in iframe src attributes and stored URLs: they must be
    // https and on an accepted domain, or anyone with this permission could
    // point every player on the site somewhere else.
    $templates = array($name . '_embed_template');
    if ($name === 'bitchute') {
      $templates[] = 'bitchute_video_template';
    }
    foreach ($templates as $key) {
      $template = trim($values[$key]);
      $values[$key] = $template;
      $host = parse_url(str_replace('{id}', 'x', $template), PHP_URL_HOST);
      if (stripos($template, 'https://') !== 0 || strpos($template, '{id}') === FALSE) {
        form_set_error($key, t('@provider: the address must start with https:// and contain {id}.', array('@provider' => $title)));
      }
      elseif (!$host || !in_array(video_embed_alt_normalise_host($host), $domains, TRUE)) {
        form_set_error($key, t('@provider: the address must be on one of the accepted domains.', array('@provider' => $title)));
      }
    }
  }

  // Lookup services may live on another host (api.bitchute.com), so they are
  // held only to https.
  foreach (array('rumble_oembed_endpoint', 'bitchute_api_endpoint') as $key) {
    $values[$key] = trim($values[$key]);
    if (stripos($values[$key], 'https://') !== 0 || !filter_var($values[$key], FILTER_VALIDATE_URL)) {
      form_set_error($key, t('The lookup address must be a full https:// address.'));
    }
  }
}

/**
 * Form submission handler for video_embed_alt_settings_form().
 */
function video_embed_alt_settings_form_submit($form, &$form_state) {
  $config = config('video_embed_alt.settings');
  foreach (array_keys(video_embed_alt_settings()) as $key) {
    if (isset($form_state['values'][$key])) {
      $config->set($key, $form_state['values'][$key]);
    }
  }
  $config->save();

  // The accepted domains are part of video_embed_field's cached provider list.
  cache_clear_all('video_embed_field_handlers', 'cache');
  backdrop_set_message(t('The configuration options have been saved.'));
}

/**
 * Splits a domains textarea into a clean list.
 *
 * @param string|array $text
 *   One domain per line, or an already-parsed list.
 *
 * @return array
 *   Lowercased domains without scheme, "www." or path, duplicates removed.
 */
function video_embed_alt_parse_domains($text) {
  $lines = is_array($text) ? $text : preg_split('/[\r\n]+/', (string) $text);
  $domains = array();
  foreach ($lines as $line) {
    $line = preg_replace('#^https?://#i', '', trim($line));
    $line = video_embed_alt_normalise_host(preg_replace('#/.*$#', '', $line));
    if ($line !== '') {
      $domains[$line] = $line;
    }
  }
  return array_values($domains);
}
