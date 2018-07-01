" Vim plugin for fuspel development
" Language:     Fuspel functional programing language
" Maintainer:   Camil Staps <info@camilstaps.nl>
" License:      This file is placed in the public domain.

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let s:cpo_save = &cpo
set cpo&vim

let b:undo_ftplugin = "setlocal com< cms< fo< sua<"

setlocal comments=://
setlocal commentstring=//\ %s

setlocal formatoptions-=t formatoptions+=ro

setlocal suffixesadd=.fusp

let &cpo = s:cpo_save
unlet s:cpo_save

" vim: expandtab shiftwidth=2 tabstop=2
