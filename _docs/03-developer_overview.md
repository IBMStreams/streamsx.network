---
title: "Toolkit Development overview"
permalink: /docs/developer/overview/
excerpt: "Contributing to this toolkits development."
last_modified_at: 2017-08-04T12:37:48-04:00
redirect_from:
   - /theme-setup/
sidebar:
   nav: "developerdocs"
---
{% include toc %}
{% include editme %}

For the toolkit's source code, please see [https://github.com/IBMStreams/streamsx.network/](https://github.com/IBMStreams/streamsx.network/).

# Build the toolkit

The toolkit needs to be build before you can use it in a SPL application.
Run the following command in the `streamsx.shell` directory:

    ant all


## Running the sample applications test

Call `./autotestAll.sh` in the `sample` directory to perform the regression test.

