" Copyright 2017 The Chromium Authors. All rights reserved.
" Use of this source code is governed by a BSD-style license that can be
" found in the LICENSE file.

command! -nargs=1 CrSearch call crcs#CodeSearch(<q-args>)
command! CrXrefSearch call crcs#XrefSearch()
command! CrCallgraph call crcs#Callgraph()
command! CrLoadCallers call crcs#JumpToCallers()
command! CrShowSignature call crcs#ShowSignature()

command! -nargs=1 -complete=customlist,crcs#RefTypeCompleter CrTour call crcs#GoToRef(<q-args>)

if has_key(g:, 'codesearch_default_bindings') && g:codesearch_default_bindings
  nnoremap <leader>s :CrSearch 
  nnoremap <leader>x :CrXrefSearch<CR>
  nnoremap <leader>g :CrCallgraph<CR>
  nnoremap <leader>j :CrLoadCallers<CR>
  nnoremap <leader>gd :CrTour declarations<CR>
  nnoremap <leader>gD :CrTour definitions<CR>
  nnoremap <leader>l :CrTour 
endif

