BUILD STATUS
------------
Current build status:
[![Build Status](https://travis-ci.org/Islandora/islandora_solr_search.png?branch=7.x)](https://travis-ci.org/Islandora/islandora_solr_search)

CI Server:
http://jenkins.discoverygarden.ca

ISLANDORA SOLR SEARCH
==================

SUMMARY
-------

Islandora Solr provides a highly configurable interface to perform Solr queries and display
Solr data in different ways.


REQUIREMENTS
------------

 * Islandora
 * Apache Solr 1.4 or higher.


INSTALLATION
------------


CONFIGURATION
-------------


CUSTOMIZATION
-------------


TROUBLESHOOTING
---------------


F.A.Q.
------

 Q: Why are some dates not rendered correctly?
 
 A: On 32 bit systems, the PHP function strtotime() has a limited range. Typically
    from Fri, 13 Dec 1901 20:45:54 UTC to Tue, 19 Jan 2038 03:14:07 UTC. For more info
    see: http://php.net/manual/en/function.strtotime.php#refsect1-function.strtotime-notes
