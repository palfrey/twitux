#!/bin/zsh
for x in `git cherry HEAD git-svn|grep -v "^- "`; git cherry-pick $x
