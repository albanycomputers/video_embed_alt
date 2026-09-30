<?php

/**
 * @file
 * Rumble and BitChute providers: parsing, lookups and embed building.
 *
 * Nothing the editor types reaches the page. Every input, whether a page
 * link, a share link or pasted embed code, is reduced to a provider and a
 * video id that must match a strict pattern, and the player is rebuilt from
 * the configured embed template.
 */

/**
 * How long a successful lookup is cached, in seconds.
 *
 * Validation, video_embed_field's thumbnail step and its data step each ask
 * for the same video during one save; the cache also spares a second lookup
 * when an editor uploads an image and the form is rebuilt.
 */
define('VIDEO_EMBED_ALT_CACHE_TTL', 21600);

/**
 * Returns the provider settings, with defaults for any missing key.
 *
 * @return array
 *   The contents of video_embed_alt.settings.
 */
function video_embed_alt_settings() {
  $defaults = array(
    'rumble_domains' => array('rumble.com'),
    'rumble_embed_template' => 'https://rumble.com/embed/{id}/',
    'rumble_oembed_endpoint' => 'https://rumble.com/api/Media/oembed.json',
    'bitchute_domains' => array('bitchute.com', 'old.bitchute.com'),
    'bitchute_embed_template' => 'https://www.bitchute.com/embed/{id}/',
    'bitchute_video_template' => 'https://www.bitchute.com/video/{id}/',
    'bitchute_api_endpoint' => 'https://api.bitchute.com/api/beta9/video',
  );
  $config = config('video_embed_alt.settings')->get();
  return (is_array($config) ? $config : array()) + $defaults;
}

/**
 * Lists the providers this module adds.
 *
 * @return array
 *   Human-readable provider names, keyed by video_embed_field handler name.
 */
function video_embed_alt_providers() {
  return array(
    'rumble' => 'Rumble',
    'bitchute' => 'BitChute',
  );
}

/**
 * Lowercases a host and strips a leading "www.".
 *
 * Matches how video_embed_field compares hosts, so a domain list accepted
 * here is also accepted by its own validation.
 *
 * @param string $host
 *   A host name.
 *
 * @return string
 *   The normalised host.
 */
function video_embed_alt_normalise_host($host) {
  $host = strtolower(trim($host));
  if (strpos($host, 'www.') === 0) {
    $host = substr($host, 4);
  }
  return $host;
}

/**
 * Pulls a URL out of whatever the editor pasted.
 *
 * Accepts a bare URL, a URL without a scheme, or embed code, in which case
 * the iframe's src is used and everything else is discarded.
 *
 * @param string $input
 *   The pasted text.
 *
 * @return array|false
 *   The parse_url() parts, or FALSE if no usable URL was found.
 */
function video_embed_alt_extract_url($input) {
  $input = trim((string) $input);
  if ($input === '') {
    return FALSE;
  }

  if (strpos($input, '<') !== FALSE) {
    if (!preg_match('/\ssrc\s*=\s*["\']([^"\']+)["\']/i', $input, $matches)) {
      return FALSE;
    }
    $input = html_entity_decode($matches[1], ENT_QUOTES | ENT_HTML5, 'UTF-8');
  }

  if (strpos($input, '//') === 0) {
    $input = 'https:' . $input;
  }
  elseif (!preg_match('#^https?://#i', $input)) {
    $input = 'https://' . $input;
  }

  $parts = parse_url($input);
  // Credentials or a port in a video link are never legitimate, and are the
  // usual ingredients of a URL built to fool a host check.
  if (!$parts || empty($parts['host']) || isset($parts['user']) || isset($parts['pass']) || isset($parts['port'])) {
    return FALSE;
  }
  return $parts;
}

/**
 * Identifies the provider and video id in a pasted value.
 *
 * @param string $input
 *   A URL or embed code.
 *
 * @return array|false
 *   FALSE if the host belongs to neither provider. Otherwise an array with:
 *   - provider: 'rumble' or 'bitchute'.
 *   - id: the video id, or NULL if the host matched but the path is not a
 *     video (a channel page, say).
 *   - kind: 'embed' or 'page'. Rumble only: its page id and embed id differ.
 *   - page_url: Rumble page links only, the cleaned URL to look up.
 */
function video_embed_alt_identify($input) {
  $parts = video_embed_alt_extract_url($input);
  if (!$parts) {
    return FALSE;
  }

  $host = video_embed_alt_normalise_host($parts['host']);
  $path = isset($parts['path']) ? $parts['path'] : '/';
  $settings = video_embed_alt_settings();

  if (in_array($host, $settings['rumble_domains'], TRUE)) {
    $video = array('provider' => 'rumble', 'id' => NULL, 'kind' => NULL);
    if (preg_match('#^/embed/(v[a-z0-9]{3,16})/?$#i', $path, $matches)) {
      $video['id'] = $matches[1];
      $video['kind'] = 'embed';
    }
    elseif (preg_match('#^/(v[a-z0-9]{3,16})(-[^/]*?)?(\.html)?$#i', $path, $matches)) {
      $video['id'] = $matches[1];
      $video['kind'] = 'page';
      // Rumble's oEmbed needs the ".html" form and 404s on a bare id, but
      // ignores the slug, so an empty one ("v123-.html") is enough. The slug
      // is reduced to safe characters because this URL is sent to Rumble.
      $slug = isset($matches[2]) ? preg_replace('/[^a-z0-9-]/i', '', $matches[2]) : '';
      $video['page_url'] = 'https://' . $host . '/' . $video['id'] . ($slug === '' ? '-' : $slug) . '.html';
    }
    return $video;
  }

  if (in_array($host, $settings['bitchute_domains'], TRUE)) {
    $video = array('provider' => 'bitchute', 'id' => NULL, 'kind' => 'page');
    if (preg_match('#^/(?:video|embed)/([A-Za-z0-9_-]{6,20})/?$#', $path, $matches)) {
      $video['id'] = $matches[1];
    }
    return $video;
  }

  return FALSE;
}

/**
 * Builds a URL from a configured template.
 *
 * @param string $template
 *   A template containing {id}.
 * @param string $id
 *   A video id that has already passed its provider's pattern.
 *
 * @return string
 *   The URL.
 */
function video_embed_alt_build_url($template, $id) {
  return str_replace('{id}', rawurlencode($id), $template);
}

/**
 * Resolves a pasted value to its canonical URL and details.
 *
 * Results are cached per video. A failure is never cached, so a provider
 * that was briefly unreachable is asked again on the next attempt.
 *
 * @param string $input
 *   A URL or embed code.
 *
 * @return array|false
 *   FALSE if the value is not a video from either provider, or the provider
 *   says the video does not exist. Otherwise an array with:
 *   - provider, id: the provider and the id used to build the embed.
 *   - url: the canonical URL to store in the field.
 *   - link: where the "watch on" link should point.
 *   - title, thumbnail_url: may be empty if the lookup failed.
 */
function video_embed_alt_resolve($input) {
  $video = video_embed_alt_identify($input);
  if (!$video || empty($video['id'])) {
    return FALSE;
  }

  $cid = 'video_embed_alt:' . $video['provider'] . ':' . $video['kind'] . ':' . $video['id'];
  $results = &backdrop_static(__FUNCTION__, array());
  if (isset($results[$cid])) {
    return $results[$cid];
  }
  if ($cached = cache_get($cid)) {
    return $results[$cid] = $cached->data;
  }

  $function = 'video_embed_alt_' . $video['provider'] . '_resolve';
  $result = $function($video);
  if ($result) {
    $results[$cid] = $result;
    if (!empty($result['thumbnail_url'])) {
      cache_set($cid, $result, 'cache', REQUEST_TIME + VIDEO_EMBED_ALT_CACHE_TTL);
      // A Rumble page link resolves to an embed id, and the embed URL is what
      // gets stored; later steps of the same save ask for that one.
      if ($video['kind'] === 'page' && $result['provider'] === 'rumble') {
        $embed_cid = 'video_embed_alt:rumble:embed:' . $result['id'];
        $results[$embed_cid] = $result;
        cache_set($embed_cid, $result, 'cache', REQUEST_TIME + VIDEO_EMBED_ALT_CACHE_TTL);
      }
    }
  }
  return $result;
}

/**
 * Resolves a Rumble video through Rumble's oEmbed endpoint.
 *
 * @param array $video
 *   As returned by video_embed_alt_identify().
 *
 * @return array|false
 *   See video_embed_alt_resolve().
 */
function video_embed_alt_rumble_resolve(array $video) {
  $settings = video_embed_alt_settings();
  $lookup = $video['kind'] === 'embed'
    ? video_embed_alt_build_url($settings['rumble_embed_template'], $video['id'])
    : $video['page_url'];

  $endpoint = $settings['rumble_oembed_endpoint'];
  $endpoint .= (strpos($endpoint, '?') === FALSE ? '?' : '&') . http_build_query(array('url' => $lookup));
  $response = video_embed_alt_http_json($endpoint);

  $embed_id = NULL;
  if ($response['data'] && !empty($response['data']['html'])
    && preg_match('#/embed/(v[a-z0-9]{3,16})/#i', $response['data']['html'], $matches)) {
    $embed_id = $matches[1];
  }

  if (!$embed_id) {
    // A page link cannot be embedded without the lookup: its id is not the
    // embed id. An embed link already carries the right id, so only a
    // definite "no such video" rejects it; any other failure keeps the video
    // and loses only the title and thumbnail.
    if ($video['kind'] === 'page' || $response['code'] == 404) {
      return FALSE;
    }
    $embed_id = $video['id'];
  }

  $url = video_embed_alt_build_url($settings['rumble_embed_template'], $embed_id);
  return array(
    'provider' => 'rumble',
    'id' => $embed_id,
    'url' => $url,
    // oEmbed does not return the page URL, and the embed URL is what is
    // stored, so the link goes to Rumble's own full-window player.
    'link' => $url,
    'title' => isset($response['data']['title']) ? (string) $response['data']['title'] : '',
    'thumbnail_url' => video_embed_alt_clean_thumbnail(isset($response['data']['thumbnail_url']) ? $response['data']['thumbnail_url'] : ''),
  );
}

/**
 * Resolves a BitChute video through BitChute's website API.
 *
 * The endpoint is undocumented (it is what bitchute.com's own pages call),
 * so any failure keeps the video and loses only the title and thumbnail: the
 * embed is built from the id in the link and needs nothing from the API.
 *
 * @param array $video
 *   As returned by video_embed_alt_identify().
 *
 * @return array|false
 *   See video_embed_alt_resolve().
 */
function video_embed_alt_bitchute_resolve(array $video) {
  $settings = video_embed_alt_settings();
  $response = video_embed_alt_http_json($settings['bitchute_api_endpoint'], array(
    'method' => 'POST',
    'data' => json_encode(array('video_id' => $video['id'])),
    'headers' => array('Content-Type' => 'application/json'),
  ));

  $data = $response['data'];
  // Only trust a reply about the video we asked for.
  if (!$data || !isset($data['video_id']) || $data['video_id'] !== $video['id']) {
    $data = array();
  }

  return array(
    'provider' => 'bitchute',
    'id' => $video['id'],
    'url' => video_embed_alt_build_url($settings['bitchute_video_template'], $video['id']),
    'link' => video_embed_alt_build_url($settings['bitchute_video_template'], $video['id']),
    'title' => isset($data['video_name']) ? (string) $data['video_name'] : '',
    'thumbnail_url' => video_embed_alt_clean_thumbnail(isset($data['thumbnail_url']) ? $data['thumbnail_url'] : ''),
  );
}

/**
 * Makes an HTTP request expecting JSON.
 *
 * @param string $url
 *   The URL to request.
 * @param array $options
 *   Options for backdrop_http_request().
 *
 * @return array
 *   An array with 'code' (the HTTP status, negative on a network error) and
 *   'data' (the decoded JSON array, or NULL).
 */
function video_embed_alt_http_json($url, array $options = array()) {
  $options += array('timeout' => 8);
  $options['headers'] = (isset($options['headers']) ? $options['headers'] : array()) + array('Accept' => 'application/json');

  $response = backdrop_http_request($url, $options);
  $data = NULL;
  if (empty($response->error) && $response->code == 200 && !empty($response->data)) {
    $decoded = json_decode($response->data, TRUE);
    if (is_array($decoded)) {
      $data = $decoded;
    }
  }
  return array('code' => (int) $response->code, 'data' => $data);
}

/**
 * Lists the image types a thumbnail may be.
 *
 * SVG is deliberately absent: it can carry script, and image_get_info()
 * recognises it.
 *
 * @return array
 *   Lowercase extensions.
 */
function video_embed_alt_thumbnail_extensions() {
  return array('jpg', 'jpeg', 'png', 'gif', 'webp');
}

/**
 * Accepts a thumbnail URL only if it is an https URL to an image file.
 *
 * video_embed_field names the file it downloads after the URL's extension,
 * so a URL ending in ".php" from a compromised or changed provider API would
 * otherwise land in public files under that name.
 *
 * @param mixed $url
 *   The value a provider returned.
 *
 * @return string
 *   The URL, or '' if it is not acceptable.
 */
function video_embed_alt_clean_thumbnail($url) {
  if (!is_string($url) || !filter_var($url, FILTER_VALIDATE_URL) || stripos($url, 'https://') !== 0) {
    return '';
  }
  $extension = strtolower(pathinfo((string) parse_url($url, PHP_URL_PATH), PATHINFO_EXTENSION));
  if (!in_array($extension, video_embed_alt_thumbnail_extensions(), TRUE)) {
    return '';
  }
  return $url;
}

/**
 * Builds the player markup for a stored Rumble or BitChute URL.
 *
 * Called by video_embed_field on every render, so it must not make a
 * request. The stored URL is canonical and already carries the embed id; a
 * Rumble page link, which can only arrive through a programmatic save, is
 * resolved through the cache as a fallback.
 *
 * @param string $url
 *   The stored video URL.
 *
 * @return array
 *   A render array: the responsive player, or a plain link if the URL cannot
 *   be understood.
 */
function video_embed_alt_player($url) {
  $video = video_embed_alt_identify($url);
  if ($video && !empty($video['id']) && $video['provider'] === 'rumble' && $video['kind'] === 'page') {
    $resolved = video_embed_alt_resolve($url);
    $video = $resolved ? array('provider' => 'rumble', 'id' => $resolved['id']) : FALSE;
  }
  if (!$video || empty($video['id'])) {
    return array('#markup' => l($url, $url));
  }

  $settings = video_embed_alt_settings();
  $providers = video_embed_alt_providers();
  $src = video_embed_alt_build_url($settings[$video['provider'] . '_embed_template'], $video['id']);

  $attributes = array(
    'src' => $src,
    'title' => t('@provider video', array('@provider' => $providers[$video['provider']])),
    'loading' => 'lazy',
    'allow' => 'fullscreen; picture-in-picture',
    'allowfullscreen' => 'allowfullscreen',
  );
  return array(
    '#markup' => '<div class="vea-frame"><iframe' . backdrop_attributes($attributes) . '></iframe></div>',
    '#attached' => array(
      'css' => array(backdrop_get_path('module', 'video_embed_alt') . '/css/video_embed_alt.css'),
    ),
  );
}

/**
 * Embed callback for the Rumble handler.
 *
 * @param string $url
 *   The stored video URL.
 * @param array $settings
 *   Video style settings. Unused: the player is always responsive 16:9.
 *
 * @return array
 *   A render array.
 */
function video_embed_alt_handle_rumble($url, $settings) {
  return video_embed_alt_player($url);
}

/**
 * Embed callback for the BitChute handler.
 *
 * @param string $url
 *   The stored video URL.
 * @param array $settings
 *   Video style settings. Unused: the player is always responsive 16:9.
 *
 * @return array
 *   A render array.
 */
function video_embed_alt_handle_bitchute($url, $settings) {
  return video_embed_alt_player($url);
}

/**
 * Thumbnail callback for both handlers.
 *
 * video_embed_field downloads the returned URL when the entity is saved.
 *
 * @param string $url
 *   The stored video URL.
 *
 * @return array
 *   'id' and 'url' of the thumbnail, or an empty array if there is none.
 */
function video_embed_alt_handle_thumbnail($url) {
  $resolved = video_embed_alt_resolve($url);
  if (!$resolved || empty($resolved['thumbnail_url'])) {
    return array();
  }
  return array('id' => $resolved['id'], 'url' => $resolved['thumbnail_url']);
}

/**
 * Data callback for both handlers.
 *
 * The result is stored with the field item, so the formatter can show the
 * title and link without asking the provider again.
 *
 * @param string $url
 *   The stored video URL.
 *
 * @return array
 *   Title, link and thumbnail URL, or an empty array.
 */
function video_embed_alt_handle_data($url) {
  $resolved = video_embed_alt_resolve($url);
  if (!$resolved) {
    return array();
  }
  return array(
    'title' => $resolved['title'],
    'link' => $resolved['link'],
    'thumbnail_url' => $resolved['thumbnail_url'],
  );
}
