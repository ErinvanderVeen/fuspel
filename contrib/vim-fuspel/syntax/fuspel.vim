" Fuspel syntax file
" Language:     Fuspel functional programing language
" Author:       Camil Staps <info@camilstaps.nl>
" License:      This file is placed in the public domain.

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn keyword fuspelKeyword    code main import

syn keyword fuspelTodo       TODO FIXME XXX BUG NB contained containedin=fuspelComment
syn region  fuspelComment    start="//" end="$" contains=@Spell oneline display

syn match   fuspelInteger    /\d\+/ display

syn match   fuspelDelimiter  /\v[\[\]\(\):;=,]/ display

syn match   fuspelIdentifier /^[_a-zA-Z]\+/ display

hi def link fuspelKeyword    Keyword
hi def link fuspelInteger    Number
hi def link fuspelDelimiter  Delimiter
hi def link fuspelIdentifier Identifier
hi def link fuspelTodo       Todo
hi def link fuspelComment    Comment

syntax sync ccomment fuspelComment
setlocal foldmethod=syntax

let b:current_syntax = 'fuspel'

let &cpo = s:cpo_save
unlet s:cpo_save
