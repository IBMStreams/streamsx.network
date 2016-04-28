#!/bin/bash

## Copyright (C) 2016 International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

self=$( basename $0 .sh )
here=$( cd ${0%/*} ; pwd )

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

cd $here || die "sorry, could not get to '$here' ..."

url=$( git config --get remote.origin.url )
clone=/tmp/$self\@$USER

step "pushing documentation from '$here/doc' to '$url' ..."
[ -d $here/doc ] || die "sorry, could not find $here/doc"

step "cloning '$url' into '$clone' ..."
[ ! -d $clone ] || rm -rf $clone || die "sorry, could not delete old $clone"
mkdir -p $clone || die "sorry, could not create $clone"
cd $clone || die "sorry, could not go to $clone"
git clone $url || die "sorry, could not clone $url ..."

repository=$( ls -1 )

step "switching to 'gh-pages' branch of cloned repository '$repository' ..."
cd $clone/$repository || die "sorry, could not go to $clone/$repository"
git checkout gh-pages || die "sorry, could not switch to 'gh-pages' branch of $url ..."

step "copying documentation files from '$here/doc' to '$clone/$repository' ..."
cp -r -p $here/doc $clone/$repository || die "sorry, could not copy from from $here/doc to $clone/$repository"
git add $clone/$repository/doc --all || die "sorry, could not add files to git index"
#count=$( git status -s | wc -l )
echo $( git status -s | wc -l ) "new/changed/deleted documentation files"
git commit -a -m "update toolkit documentation" || die "sorry, nothing to commit to git"

step "pushing new/modified/deleted files to '$url' ..."
git push origin gh-pages || die "sorry, could not push files to $url"

step "deleting '$clone' ..."
rm -rf $clone || die "sorry, could not delete $clone"

exit 0
