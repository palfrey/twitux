#!/bin/zsh
for x in `git cherry HEAD git-svn`; git cherry-pick acbe45386780b1f7a0e5d8199695cd9afd06a70a
