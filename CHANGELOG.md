# Changelog

## 1.x-1.0.0 (unreleased)

First version.

- Rumble and BitChute providers for Video Embed Field. Accepts page links,
  share links and embed code; stores a canonical address and rebuilds the
  player from a configured template.
- Rumble page ids are resolved to embed ids through Rumble's oEmbed service
  on save. Title and thumbnail come from the same lookup.
- BitChute title and thumbnail from BitChute's website API (undocumented;
  failure only loses the thumbnail).
- "Video player, click-to-load" formatter with three load modes, including
  one that follows EU Cookie Compliance.
- Consent-mode cards say why the player has not loaded: a different
  message before the reader answers the cookie banner and after declining.
- Copies the video thumbnail into a chosen image field; refuses the save
  when no thumbnail is available and the image field is empty.
- Settings page for accepted domains, embed templates and lookup addresses.
