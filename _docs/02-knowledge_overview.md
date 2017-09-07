---
title: "Toolkit technical background overview"
permalink: /docs/knowledge/overview/
excerpt: "Basic knowledge of the toolkits technical domain."
last_modified_at: 2017-08-04T12:37:48-04:00
redirect_from:
   - /theme-setup/
sidebar:
   nav: "knowledgedocs"
---
{% include toc %}
{% include editme %}


The Network toolkit for IBM Streams enables SPL applications to process low-level network traffic.

It includes operators and functions that can:

* ingest raw ethernet packets from live network interfaces and recordings of network traffic
* parse DHCP, DNS, Netflow, and IPFIX messages
* reassemble application-level files from network packets
* map IP addresses into city, state, and country locations
* convert IP addresses betweeen binary and string representations

This version of the toolkit is intended for use with IBM Streams release 4.1 and later.
