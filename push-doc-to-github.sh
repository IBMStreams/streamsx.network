#!/bin/bash

## Copyright (C) 2016, 2017 International Business Machines Corporation
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

cd $here || die "sorry, could not go to directory $here"

# generate SPLDOC files in local copy of repository

step "generating SPLDOC for toolkit in directory $here ..."
directories=( $( find $here -name 'info.xml' -exec dirname {} \; ) )
toolkits=$( IFS=":" ; echo "${directories[*]}" )
spl-make-doc --output-directory $here/doc/spldoc --toolkit-path $toolkits --check-tags --include-all || die "sorry, spl-make-doc failed on $here/doc/spldoc"

# clone another temporary copy of toolkit from GitHub 

url=$( git config --get remote.origin.url )
clone=/tmp/$self\@$USER
step "creating temporary clone of repository '$url' in directory '$clone' ..."
[[ ! -d $clone ]] || rm -rf $clone || die "sorry, could not delete old directory $clone"
mkdir -p $clone || die "sorry, could not create directory $clone"
cd $clone || die "sorry, could not go to directory $clone"
git clone $url || die "sorry, could not clone repository $url ..."

# check out 'gh-pages' branch of temporary clone of repository

repository=$( ls -1 )
step "switching to 'gh-pages' branch of temporary copy of repository '$repository' ..."
cd $clone/$repository || die "sorry, could not go to directory $clone/$repository"
git checkout gh-pages || die "sorry, could not check out 'gh-pages' branch of $url"

# copy generated SPLDOC files into temporary clone of repository

step "copying documentation files from '$here/doc' to '$clone/$repository' ..."
cp -r $here/doc $clone/$repository || die "sorry, could not copy from from $here/doc to $clone/$repository"
git add $clone/$repository/doc --all || die "sorry, could not add documentatino files to git index"
echo $( git status -s | wc -l ) "new/changed/deleted documentation files"

# push generated SPLDOC files to GitHub

step "pushing new/modified/deleted documentation files to '$url' ..."
git commit -a -m "update toolkit documentation" || die "sorry, nothing to commit to git"
git push origin gh-pages || die "sorry, could not push files to $url"

# clean up

step "deleting temporary clone of repository in directory '$clone' ..."
rm -rf $clone || die "sorry, could not delete $clone"

exit 0
