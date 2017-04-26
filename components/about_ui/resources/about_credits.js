// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function $(id) { return document.getElementById(id); }

function toggle(o) {
  var licence = o.nextSibling;

  while (licence.className != 'licence') {
    if (!licence) return false;
    licence = licence.nextSibling;
  }

  if (licence.style && licence.style.display == 'block') {
    licence.style.display = 'none';
    o.textContent = 'show license';
  } else {
    licence.style.display = 'block';
    o.textContent = 'hide license';
  }
  return false;
}

document.addEventListener('DOMContentLoaded', function() {
  var licenseEls = [].slice.call(document.getElementsByClassName('product'));

  licenseEls.sort(function(a, b) {
    let nameA = a.getElementsByClassName('title')[0].textContent;
    let nameB = b.getElementsByClassName('title')[0].textContent;
    if (nameA < nameB) {
      return -1;
    }

    if (nameA > nameB) {
      return 1;
    }

    return 0;
  });

  let parentDiv = licenseEls[0].parentNode;
  parentDiv.innerHTML = '';
  for (let i = 0; i < licenseEls.length; i++) {
    parentDiv.appendChild(licenseEls[i]);
  }

  document.body.style.display = '';

  if (cr.isChromeOS) {
    var keyboardUtils = document.createElement('script');
    keyboardUtils.src = 'chrome://credits/keyboard_utils.js';
    document.body.appendChild(keyboardUtils);
  }

  var links = document.querySelectorAll('a.show');
  for (var i = 0; i < links.length; ++i) {
    links[i].onclick = function() { return toggle(this); };
  }

  $('print-link').onclick = function() {
    window.print();
    return false;
  };
});
