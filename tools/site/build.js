/**
 * Static-site generator for the chaosircd GitHub Pages site.
 *
 *   qjsm tools/site/build.js [outdir]      (default outdir: docs)
 *   node tools/site/build.js [outdir]      (also works)
 *
 * Renders the repo's own markdown (README.md, IRCv3.md) into a
 * self-contained HTML site: a hand-written landing page and every doc page
 * behind a shared shell with sidebar navigation and a per-page table of
 * contents. Inter-doc *.md links are rewritten to their generated pages;
 * links that point at anything else in the repo (src/, modules/, COPYING,
 * ...) are rewritten to github.com blob/tree URLs.
 *
 * Everything is relative-path linked so the site works both at
 * https://rsenn.github.io/chaosircd/ and from a local file:// checkout.
 *
 * Adapted from ../../../shish/tools/site/build.js - markdown.js,
 * highlight.js, style.css and publish.sh are shared verbatim; this file,
 * landing.html and favicon.svg are chaosircd's own.
 */

import * as fs from 'fs';
import { render } from './markdown.js';
import { highlight } from './highlight.js';

const REPO = 'rsenn/chaosircd';
const GITHUB = 'https://github.com/' + REPO;
const TAGLINE = 'an IRC daemon with (re)loadable modules';

/* --------------------------------------------------------------- site map */

const NAV = [
  {
    group: 'Start here',
    pages: [['README.md', 'getting-started.html', 'Getting started']],
  },
  {
    group: 'Reference',
    pages: [['IRCv3.md', 'docs/ircv3.html', 'IRCv3 migration plan']],
  },
];

const PAGES = [];
for (const { group, pages } of NAV)
  for (const [src, out, title] of pages) PAGES.push({ src, out, title, group });

const bySrc = new Map(PAGES.map(p => [p.src, p]));

/* ------------------------------------------------------------ path helpers */

const dirname = p => (p.indexOf('/') >= 0 ? p.slice(0, p.lastIndexOf('/')) : '');

function normalize(p) {
  const out = [];
  for (const part of p.split('/')) {
    if (!part || part === '.') continue;
    if (part === '..') out.pop();
    else out.push(part);
  }
  return out.join('/');
}

/** Path from a directory to a file, both site-relative. */
function relative(fromDir, to) {
  const f = fromDir ? fromDir.split('/') : [];
  const t = to.split('/');
  let i = 0;
  while (i < f.length && i < t.length - 1 && f[i] === t[i]) i++;
  return '../'.repeat(f.length - i) + t.slice(i).join('/');
}

function mkdirp(path) {
  let cur = path.startsWith('/') ? '/' : '';
  for (const part of path.split('/')) {
    if (!part) continue;
    cur += (cur ? '/' : '') + part;
    if (!fs.existsSync(cur)) fs.mkdirSync(cur, 0o755);
  }
}

function read(path) {
  return fs.readFileSync(path, 'utf8');
}

function write(path, text) {
  mkdirp(dirname(path));
  fs.writeFileSync(path, text);
}

function copy(from, to) {
  mkdirp(dirname(to));
  fs.copyFileSync(from, to);
}

/* --------------------------------------------------------- link rewriting */

/** Rewrite one markdown href found in `page` into a link that works on the site. */
function linkFor(page, href) {
  if (!href || href.startsWith('#') || href.startsWith('//') || /^[a-z][a-z0-9+.-]*:/i.test(href))
    return href;

  const hash = href.indexOf('#');
  const path = hash < 0 ? href : href.slice(0, hash);
  const frag = hash < 0 ? '' : href.slice(hash);
  if (!path) return href;

  const target = normalize(dirname(page.src) + '/' + path);
  const hit = bySrc.get(target);
  if (hit) return relative(dirname(page.out), hit.out) + frag;

  const kind = path.endsWith('/') ? 'tree' : 'blob';
  return GITHUB + '/' + kind + '/main/' + target + frag;
}

/* ------------------------------------------------------------- html shell */

const escAttr = s => s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/"/g, '&quot;');

function sidebar(page) {
  const here = dirname(page.out);
  let html = '';
  for (const { group, pages } of NAV) {
    html += '<div class="navgroup"><h3>' + group + '</h3><ul>';
    for (const [, out, title] of pages) {
      const cls = out === page.out ? ' class="here"' : '';
      html += '<li><a href="' + escAttr(relative(here, out)) + '"' + cls + '>' + title + '</a></li>';
    }
    html += '</ul></div>';
  }
  return html;
}

function toc(headings) {
  const items = headings.filter(h => h.level >= 2 && h.level <= 3);
  if (items.length < 2) return '';
  const links = items
    .map(
      h =>
        '<li class="lvl' + h.level + '"><a href="#' + escAttr(h.id) + '">' + escAttr(h.text) + '</a></li>')
    .join('');
  return '<nav class="toc"><h3>On this page</h3><ul>' + links + '</ul></nav>';
}

function shell({ title, root, body, cls, head }) {
  return `<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>${escAttr(title)}</title>
<meta name="description" content="chaosircd — ${escAttr(TAGLINE)}. Loadable command/chanmode/usermode modules, a dedicated DNS/ident/proxy-scan process, and an in-process WebSocket gateway for browser clients.">
<link rel="stylesheet" href="${root}assets/style.css">
<link rel="icon" href="${root}assets/favicon.svg" type="image/svg+xml">
<script>try{var t=localStorage.getItem('theme');if(t)document.documentElement.dataset.theme=t}catch(e){}</script>
${head || ''}</head>
<body class="${cls}">
<header class="topbar">
  <a class="brand" href="${root}index.html"><span class="mark">#</span> chaosircd</a>
  <nav class="topnav">
    <a href="${root}getting-started.html">Get started</a>
    <a href="${root}docs/ircv3.html">IRCv3 plan</a>
    <a href="${GITHUB}" target="_blank" rel="noopener">GitHub</a>
  </nav>
  <button class="themetoggle" type="button" aria-label="Toggle colour scheme">◐</button>
</header>
${body}
<footer class="sitefoot">
  <p>chaosircd — GPL v2 (LGPL v2 for the core library). Built from the repo's own markdown by
     <a href="${GITHUB}/blob/main/tools/site/build.js">tools/site/build.js</a>.</p>
</footer>
<script>
document.querySelector('.themetoggle').addEventListener('click', function () {
  var d = document.documentElement;
  var dark = d.dataset.theme ? d.dataset.theme === 'dark'
    : matchMedia('(prefers-color-scheme: dark)').matches;
  d.dataset.theme = dark ? 'light' : 'dark';
  try { localStorage.setItem('theme', d.dataset.theme); } catch (e) {}
});
</script>
</body>
</html>
`;
}

/* ------------------------------------------------------------------ build */

function buildPage(page) {
  const md = read(page.src);
  const { html, headings } = render(md, {
    link: href => linkFor(page, href),
    highlight,
  });

  const depth = page.out.split('/').length - 1;
  const root = '../'.repeat(depth);
  const h1 = headings.find(h => h.level === 1);

  const body = `<div class="layout">
<aside class="sidebar">${sidebar(page)}</aside>
<main class="doc">
<article>${html}</article>
<p class="editlink"><a href="${GITHUB}/blob/main/${page.src}">Edit this page on GitHub →</a></p>
</main>
${toc(headings)}
</div>`;

  write(OUT + '/' + page.out, shell({
    title: (h1 ? h1.text : page.title) + ' — chaosircd',
    root, body, cls: 'has-sidebar',
  }));
}

function buildLanding() {
  const body = read(SELF + '/landing.html')
    .replace(/\{\{GITHUB\}\}/g, GITHUB)
    .replace(/\{\{REPO\}\}/g, REPO);
  write(OUT + '/index.html', shell({
    title: 'chaosircd — ' + TAGLINE,
    root: '', body, cls: 'landing',
  }));
}

const SELF = dirname(import.meta.url.replace(/^file:\/\//, '')) || '.';
const OUT = process.argv[2] || 'docs';

buildLanding();
for (const page of PAGES) buildPage(page);

copy(SELF + '/style.css', OUT + '/assets/style.css');
copy(SELF + '/favicon.svg', OUT + '/assets/favicon.svg');

console.log('Built ' + (PAGES.length + 1) + ' pages into ' + OUT + '/');
