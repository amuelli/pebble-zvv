module.exports = function(minified) {
  var clayConfig = this;
  var $ = minified.$;

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    var favsItem = clayConfig.getItemByMessageKey('FAVORITES');
    favsItem.hide();

    var favs = [];
    var MAX_FAVS = 10;
    var debounceTimer = null;

    var stored = favsItem.get();
    if (!stored) {
      try { stored = localStorage.getItem('zvv_favorites'); } catch(e) {}
    }
    if (stored) { try { favs = JSON.parse(stored); } catch(e) { favs = []; } }

    function cleanName(name) {
      return name
        .replace(/^Basel, /, '')
        .replace(/^Bern, /, '')
        .replace(/^St\.Gallen, /, '')
        .replace(/^Z\u00fcrich, /, '')
        .replace(/Bahnhof/, 'Bhf');
    }

    function esc(s) {
      return String(s)
        .replace(/&/g, '&amp;').replace(/</g, '&lt;')
        .replace(/>/g, '&gt;').replace(/"/g, '&quot;');
    }

    function persist() {
      favsItem.set(JSON.stringify(favs));
    }

    function renderFavs() {
      var html = '';
      if (favs.length === 0) {
        html = '<li style="color:#aaa;padding:8px 0;font-size:14px">No favorites yet.</li>';
      } else {
        for (var i = 0; i < favs.length; i++) {
          html += '<li style="display:flex;align-items:center;justify-content:space-between;padding:8px 0;border-bottom:1px solid #eee">' +
            '<span>' + esc(favs[i].name) + '</span>' +
            '<button data-idx="' + i + '" style="min-width:auto;margin:0 0 0 auto" class="button zvv-rm" type="button">Remove</button>' +
            '</li>';
        }
      }
      if (favs.length >= MAX_FAVS) {
        html += '<li style="color:#ff3b30;font-size:13px;padding:4px 0">Maximum 10 favorites reached.</li>';
      }
      document.getElementById('zvv-favs').innerHTML = html;
    }

    function renderResults(stations) {
      var html = '';
      for (var i = 0; i < stations.length; i++) {
        html += '<li style="display:flex;align-items:center;justify-content:space-between;padding:8px 0;border-bottom:1px solid #eee">' +
          '<span>' + esc(stations[i].name) + '</span>' +
          '<button data-id="' + esc(stations[i].id) + '" data-name="' + esc(stations[i].name) + '" ' +
          'style="min-width:auto;margin:0 0 0 auto" class="button primary zvv-add" type="button">Add</button>' +
          '</li>';
      }
      document.getElementById('zvv-results').innerHTML = html;
    }

    function doSearch(term) {
      if (!term || term.length < 2) {
        document.getElementById('zvv-results').innerHTML = '';
        return;
      }
      $.request('get', 'https://transport.opendata.ch/v1/locations?query=' + encodeURIComponent(term))
        .then(function(result) {
          try {
            var data = JSON.parse(result);
            var stations = (data.stations || [])
              .filter(function(s) { return s.id != null; })
              .slice(0, 5)
              .map(function(s) { return { id: s.id, name: cleanName(s.name || '') }; });
            renderResults(stations);
          } catch(e) {}
        })
        .error(function() { document.getElementById('zvv-results').innerHTML = ''; });
    }

    var style = document.createElement('style');
    style.textContent = '#zvv-search::-webkit-search-cancel-button { filter: brightness(0) invert(1); }';
    document.head.appendChild(style);

    clayConfig.getItemById('station-picker').set(
      '<div style="padding:4px 0">' +
        '<div class="component-input"><input id="zvv-search" type="search" placeholder="Search for a station\u2026" autocomplete="off"></div>' +
        '<ul id="zvv-results" style="list-style:none;margin:4px 0 0;padding:0"></ul>' +
        '<p style="font-size:0.85em;text-transform:uppercase;color:#888;margin:12px 0 6px">Favorites</p>' +
        '<ul id="zvv-favs" style="list-style:none;margin:0;padding:0"></ul>' +
      '</div>'
    );

    // Use native event delegation — reliable with dynamically injected HTML
    document.getElementById('zvv-results').addEventListener('click', function(e) {
      var btn = e.target;
      if (btn.className.indexOf('zvv-add') === -1) return;
      if (favs.length >= MAX_FAVS) return;
      var id = btn.getAttribute('data-id');
      var name = btn.getAttribute('data-name');
      for (var i = 0; i < favs.length; i++) { if (favs[i].id === id) return; }
      favs.push({ id: id, name: name });
      renderFavs();
      persist();
      document.getElementById('zvv-search').value = '';
      document.getElementById('zvv-results').innerHTML = '';
    });

    document.getElementById('zvv-favs').addEventListener('click', function(e) {
      var btn = e.target;
      if (btn.className.indexOf('zvv-rm') === -1) return;
      favs.splice(parseInt(btn.getAttribute('data-idx'), 10), 1);
      renderFavs();
      persist();
    });

    document.getElementById('zvv-search').addEventListener('input', function(e) {
      var term = e.target.value.trim();
      clearTimeout(debounceTimer);
      debounceTimer = setTimeout(function() { doSearch(term); }, 350);
    });

    renderFavs();
  });
};
