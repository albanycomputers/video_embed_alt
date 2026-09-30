/**
 * @file
 * Click-to-load and consent-aware loading for Video Embed Alt players.
 *
 * Each card holds its player in a <template>, which the browser does not
 * render, so nothing is requested from the video site until the player is
 * moved into the page. That happens when the reader clicks, or - for cards
 * in "consent" mode - as soon as the reader has accepted cookies through
 * EU Cookie Compliance, including on a later page view.
 */

(function ($) {

  'use strict';

  /**
   * Reports whether the reader has accepted cookies.
   *
   * @return {boolean}
   *   TRUE only if EU Cookie Compliance is present and says so. Without the
   *   module there is no answer to follow, so consent cards stay
   *   click-to-load.
   */
  function hasConsent() {
    var ecc = Backdrop.eu_cookie_compliance;
    return !!(ecc && typeof ecc.hasAgreed === 'function' && ecc.hasAgreed());
  }

  /**
   * Returns the reader's answer to the cookie banner.
   *
   * @return {string}
   *   'unanswered', 'declined', or '' when EU Cookie Compliance is absent or
   *   the reader has agreed (in which case no message is needed).
   */
  function consentAnswer() {
    var ecc = Backdrop.eu_cookie_compliance;
    if (!ecc || typeof ecc.getCurrentStatus !== 'function' || hasConsent()) {
      return '';
    }
    var status = ecc.getCurrentStatus();
    if (status === null) {
      return 'unanswered';
    }
    return status === 0 ? 'declined' : '';
  }

  /**
   * Explains on a consent-mode card why the player has not loaded.
   *
   * The page arrives with a neutral note, because cached pages are the same
   * for every reader; the answer to the banner is only known here. The
   * message is shown large over the image, and also written into the note
   * below, which is then visually hidden, so screen readers hear it once.
   *
   * @param {jQuery} $card
   *   A pending consent-mode card.
   */
  function explainConsent($card) {
    var args = { '@provider': $card.attr('data-vea-provider') };
    var answer = consentAnswer();
    var text = '';
    if (answer === 'unanswered') {
      text = Backdrop.t('This video is from @provider, which sets third-party cookies. Accept cookies to load videos automatically, or click the video to load just this one.', args);
    }
    else if (answer === 'declined') {
      text = Backdrop.t('You have declined cookies, so this video has not loaded. Click the video to load it from @provider, which may set third-party cookies.', args);
    }
    if (!text) {
      return;
    }
    $card.addClass('vea-card--explained');
    $card.find('.vea-card__message').text(text);
    $card.find('.vea-card__note').text(text).addClass('element-invisible');
  }

  /**
   * Replaces a card's preview with the real player.
   *
   * @param {jQuery} $card
   *   A pending card.
   * @param {boolean} focus
   *   Move focus to the player. TRUE after a click, so a keyboard user is not
   *   left on a button that no longer exists; FALSE for automatic loads,
   *   which must not steal focus.
   */
  function activate($card, focus) {
    if (!$card.hasClass('vea-card--pending')) {
      return;
    }
    var template = $card.find('template.vea-card__player')[0];
    if (!template || !template.content) {
      return;
    }

    $card.removeClass('vea-card--pending').addClass('vea-card--loaded');
    $card.find('.vea-card__stage').replaceWith(template.content.cloneNode(true));
    $card.find('.vea-card__note').text('');
    $(template).remove();

    if (focus) {
      $card.find('iframe').first().trigger('focus');
    }
  }

  /**
   * Loads every consent-mode card still waiting on the page.
   */
  function activateConsentCards() {
    $('.vea-card--pending[data-vea-mode="consent"]').each(function () {
      activate($(this), false);
    });
  }

  Backdrop.behaviors.videoEmbedAlt = {
    attach: function (context) {
      $('.vea-card', context).once('video-embed-alt').each(function () {
        var $card = $(this);
        $card.find('.vea-card__play').on('click', function () {
          activate($card, true);
        });
        if ($card.attr('data-vea-mode') === 'consent') {
          explainConsent($card);
        }
      });

      if (hasConsent()) {
        activateConsentCards();
      }
      else {
        // The head script guessed from the cookie alone; EU Cookie Compliance
        // disagrees, so show the cards it hid.
        $('html').removeClass('vea-consented');
      }
    }
  };

  // The reader answers the banner while already on the page: load the
  // waiting players straight away rather than on the next page view.
  // Status 1 and 2 are both "agreed" in EU Cookie Compliance.
  $(document).on('eu_cookie_compliance.changeStatus', function (event, status) {
    status = parseInt(status, 10);
    if (status === 1 || status === 2) {
      activateConsentCards();
    }
    else {
      // Declined on this page: swap "accept cookies" for the declined note.
      $('.vea-card--pending[data-vea-mode="consent"]').each(function () {
        explainConsent($(this));
      });
    }
  });

})(jQuery);
