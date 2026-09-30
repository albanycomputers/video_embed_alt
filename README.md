# Video Embed Alt

Adds **Rumble** and **BitChute** to
[Video Embed Field](https://backdropcms.org/project/video_embed_field), plus:

- a **click-to-load player** that follows the reader's
  [EU Cookie Compliance](https://backdropcms.org/project/eu_cookie_compliance)
  choice, so no video site sets cookies before the reader has agreed;
- the video's **thumbnail copied into an image field** (for example the
  article image used in listings), with the editor asked to upload one when
  no thumbnail can be fetched.

## Requirements

- Video Embed Field (which needs Preset and Entity Plus)
- EU Cookie Compliance is optional. Without it, the consent-aware mode
  behaves as plain click-to-load.

**Cookie consent support:** the consent-aware mode currently works only with
[EU Cookie Compliance](https://backdropcms.org/project/eu_cookie_compliance).
Other Backdrop cookie consent modules can be added on request. Please open
an issue naming the module. With any other consent tool, videos stay
click-to-load: readers are still protected, but videos will not load
automatically after they accept cookies.

## What editors paste

Any of these works in the Video field:

- the address from the browser bar
- the link from the video's **Share** button
- the **embed code** from the Share button's Embed option

On save the value is converted to a standard address, and only the video id
is used from it. Nothing the editor pasted is ever output to the page, so the
text format never needs to allow `<iframe>`.

Rumble page links carry a different id from the one the player needs, so
saving a Rumble link asks Rumble for it. If Rumble says the video does not
exist, the save is refused with a message on the Video field.

## Setting up a content type

1. Add a **Video Embed** field. In its settings:
   - tick **Rumble** and **BitChute** (and YouTube or Vimeo if wanted) under
     *allowed video providers*;
   - choose the image field under **Article image from video thumbnail**.
     Only single-value image fields on the same content type are listed.
2. On *Manage display*, set the video field's format to
   **Video player, click-to-load**, and choose when the player loads:
   - *When cookies are accepted, otherwise click-to-load* (default)
     Until the reader agrees, the card explains why: one message before
     they have answered the cookie banner, another after declining.
   - *Always click-to-load*
   - *Immediately*

## Thumbnails and the image field

When the content is saved:

- If the image field is empty and a thumbnail was fetched, the thumbnail is
  copied into it, with the video title as its alt text.
- If the image field is empty and **no** thumbnail could be fetched, the save
  is refused with *"Could not fetch a thumbnail from … Please upload an
  image."* on the image field.
- If the editor has uploaded an image, it is always kept.

BitChute thumbnails come from a service BitChute uses for its own pages but
does not document. If it changes, BitChute videos still embed; editors are
simply asked to upload an image.

## Settings

*Configuration > Media > Rumble and BitChute embeds*
(`admin/config/media/video-embed-alt`), for users with *Administer Rumble and
BitChute embeds*.

Each provider has its accepted domains, the embed address (with `{id}` where
the video id goes) and the lookup address. Players are built from these when
a page is shown, so if a video site moves, changing the address here updates
every existing video. Keep the old domain in the accepted list after a move:
existing content still points at it.

Embed addresses must be `https://`, contain `{id}`, and be on an accepted
domain; lookup addresses must be `https://`.

Note for UK sites: use `www.bitchute.com` for BitChute embeds.
`old.bitchute.com` redirects UK visitors away from embedded players.

## Known limits

- The "Watch on Rumble" link opens Rumble's full-window player, not the
  video's page: Rumble's lookup does not return the page address.
- A reader who withdraws consent keeps any player already loaded on the
  current page until the page is reloaded.
- Some BitChute videos are not available to UK visitors. The player shows
  BitChute's own message; the link below the player is always present.
