NEW in 2.2.99.1 - unreleased
============================

  * The ~/Downloads folder is now indexed non-recursively by default.
      - This reduces the risk of Tracker unnecessarily crawling large
        directories of source code or video game data.
      - If users are storing data such as music or photos in subfolders of the
        Downloads folder, they will need to move it to an appropriate place
        (~/Music, ~/Pictures, etc.) so that it is still indexed.

Translations:

NEW in 2.2.99.0 - 2019-08-07
============================

  * Support for reading Musicbrainz metadata from audio files.
  * Tracker Writeback now uses GStreamer to write metadata to audio files,
    instead of depending on taglib directly.
  * Directories will now be ignored if they contain a file named `.nomedia`.
    A file named `.trackerignore` has the same effect, but the `.nomedia` file
    brings us in line with Android.
  * Removed obsolete 'max-media-art-width' setting.

NEW in 2.2.2 - 2019-05-02
=========================

  * Multiple leak fixes
  * Functional tests now use python3
  * Fix text extractor handling of non-existent files
  * Fix indexing of tracks in FLAC files
  * Added new tests for file formats

Translations: hr

NEW in 2.2.1 - 2019-03-06
=========================

  * Whitelist syscall fadvise64_64
  * Many fixes to functional tests
  * Fix failed functional tests being reported as successful

Translations: eo, fur, fr, it, ko, lv, nl, ru, sr

NEW in 2.2.0 - 2019-02-20
=========================

  * Disable guarantee_metadata by default. It was the case on autotools
  * Stop merging translations to schema files
  * Test suite improvements
  * Meson build improvements

Translations: cs, da, de, es, gl, hu, id, lt, pl, pt_BR, ro, sl, sv, tr, zh_TW

  Highlighted changes since 2.1.x:
  --------------------------------

  * The functionality of tracker-miner-apps has been adopted by
    tracker-miner-fs/tracker-extract.
  * All usage of deprecated TrackerSparqlBuilder is gone.

NEW in 2.2.0-alpha2 - 2019-02-06
================================
  * Fixes to desktop file indexing
  * Multiple build and compile warning fixes
  * Many test improvements and cleanups

Translations: fr

NEW in 2.2.0-alpha1 - 2018-11-13
================================

  * The functionality of tracker-miner-apps has been adopted by
    tracker-miner-fs/tracker-extract.
  * Updated tracker-miner-fs and tracker-miner-rss to use TrackerResource
  * Support for building through autotools has been removed.
  * Plugged several leaks
  * Other many build and code cleanups and fixes

Translations: ru, sk, sr

NEW in 2.1.4 - 2018-09-04
=========================

Translations: da, hr, lv

NEW in 2.1.3 - 2018-08-30
=========================

  * Unbreak watch on domain ontology owner. Fixes miners spuriously exiting.

NEW in 2.1.2 - 2018-08-30
=========================

  * Allow use of domain rules outside prefix
  * Add core-as-subproject as explicit meson option
  * Ensure utf8ness in TrackerResource helpers
  * Fix multiple cardinality issues in different extractors/properties
  * Other minor SPARQL correctness fixes

Translations: gl, id, ko

NEW in 2.1.1 - 2018-08-15
=========================

  * tracker-miner-fs: Ignore mercurial repositories
  * build: Fix build with -Werror=format-security
  * tracker-extract: Ensure metadata strings are UTF-8 in JPEG/PNG extractors
  * tracker-extract: Do not add named destinations to PDF TOCs
  * tracker-extract: Ignore XMP image metadata on PDFs
  * tracker-extract: Fix ISO 8601 date string generation on gstreamer extractor

Translations: cs, el, fr, fur, hu, lt, sl, tr, zh_TW

NEW in 2.1.0 - 2018-07-23
=========================

  * tests: Many updates and fixes to functional tests
  * tracker-extract: Better infrastructure for tests, based on JSON-LD
  * tracker-miner-fs: Restart tracker extract from the right domain
  * tracker-extract: Persistently avoid files that trigger recoverable
    errors, to avoid log spamming on restarts.

Translations: de, es, it, pl, pt_BR, ro, sv

NEW in 2.0.5 - 2018-06-25
=========================

  * tracker-extract: Made daemon able to shutdown on inactivity, lifetime
    will be managed by tracker-miner-fs
  * tracker-stract: Fixes in generated SPARQL for geolocation data
  * build: Multiple improvements and library detection fixes

Translations: el, en_GB, hr, ro, ru

NEW in 2.0.4 - 2018-02-07
=========================

  * build: Allow building tracker repo as a meson subproject
  * libtracker-common: Rename to libtracker-miners-common
  * libtracker-miners-common: Whitelist arm_fadvise64_64, getegid and getegid32 syscalls
  * tracker-extract: Add GExiv2-based extractor module for RAW files
  * tracker-extract: Blacklist gstreamer modules via plugin instead of via feature
  * tracker-extract: Blacklist video4linux2 gstreamer plugin
  * tracker-extract: Use enumerations for EXIF values
  * tracker-extract: Fix image pixel density conversions
  * tracker-miner-fs: Avoid setting rdf:types on empty files
  * meson: dependency check fixes

Translations: nb

NEW in 2.0.3 - 2017-11-14
=========================

  * meson: Multiple cleanups and fixes
  * tracker-extract: Do not overwrite nie:dataSource

NEW in 2.0.2 - 2017-10-05
=========================

  * tracker-miner-fs: Fix initialization when there's a startup timeout.
  * tracker-extract: Don't store 0-length strings on MSOffice metadata.

Translations: ca@valencia

NEW in 2.0.1 - 2017-10-04
=========================

  * tracker-miner-apps: Ignore broken .desktop file symlinks
  * tracker-miner-fs: Ignore non-native mounts
  * tracker-extract: Fix insertion of white balance values in images
  * Compile warnings and Coverity issue fixes
  * Fix srcdir!=builddir builds

Translations: fr, sl

NEW in 2.0.0 - 2017-09-12
==========================

  * Tracker shall from now on use semantic versioning
  * autotools/meson cleanups

  Translations: ca, da, de, eu, es, fur, hu, it, ko, lt, lv, nl, pt_BR, sk,
                sv, tr, zh_TW

  Overview of changes since 1.12:
  * Tracker core (store, libraries) and miners have been split, this source
    tree contains the latter.
  * tracker-miner-fs is now noticeably faster on large filesystem hierarchies.
  * tracker-miner-rss now handles enclosures.
  * Meson build instructions have been added.
  * Evolution/Thunderbird/Firefox/Nautilus plugins have been removed, they
    were either broken, unused, unmaintained, or all three.
  * All GUI (preferences, needle) has been removed too. Both were in a bad
    state of maintenance and visually outdated. The reasons to keep those
    are dubious with more capable UIs using Tracker out there.

NEW in 1.99.3 - 2017-08-22
==========================

  WARNING: This is unstable development towards 2.0. There are API and
           ABI incompatibilities that might affect you.

  * Add more assertions to testcases (CIDs 162185, 162186, 162188–162191)
  * Protect further the MP3 parser against out-of-bounds reads (CID #162187)
  * Fixed typos, and other code and build improvements.

Translations: cs, gl, id, ne, pl, sr, sr@latin

NEW in 1.99.2 - 2017-08-07
==========================

  WARNING: This is unstable development towards 2.0. There are API and
           ABI incompatibilities that might affect you.

  * Tracker miners have been split from this package, all tracker libraries
    and infrastructure remain at: https://git.gnome.org/browse/tracker
  * Support for RSS enclosures have been added to the RSS miner.

NEW in 1.99.1 - 2017-07-19
==========================

  WARNING: This is unstable development towards 2.0. There are API and
           ABI incompatibilities that might affect you.

  * Notable speedups to tracker-miner-fs, main loop overhead was greatly
    reduced by processing elements in batches. Indexing has been observed
    to be up to 2x faster, and startup on an indexed and up-to-date
    filesystem up to 3x.
  * More notable speedups to tracker-miner-fs startup, this applies only
    to filesystems where the number of indexed folders exceed the amount
    of inotify handles. Inotify monitoring is temporarily disabled during
    filesystem mtime checks, resulting in up to 4x faster startup. (In
    addition to the previous point).
  * Refurbished the allocation scheme for underlying DB interfaces. The
    benefit is twofold, this makes TrackerSparqlConnections truly isolated
    instances, and results in much reduced mutex contention on stress
    situations.
  * Dropped deprecated API to get direct/bus connections. Use
    tracker_sparql_connection_get().
  * Deprecated TrackerSparqlBuilder. Use TrackerResource.
  * Added tracker_sparql_connection_get_namespace_manager() to fetch the
    namespaces as per the ontology of the connection.
  * Dropped support for non-standard SPARQL syntax "AS var", the right
    syntax is "AS ?var", defined in SPARQL1.1 and accepted by Tracker
    for a long time.
  * Added tracker:title-order() sparql function, only meant to be used
    in "ORDER BY" clauses. It drops the common articles at the beginning
    of the given variable for sorting purposes.
  * Fix shutdown issues on tracker-store introduced in 1.99.0. No more
    spurious integrity checks on startup.
  * Misc code and build fixes.

Translations: fur, id, sk

NEW in 1.99.0 - 2017-07-06
==========================

  WARNING: This is unstable development towards 2.0. There are API and
           ABI incompatibilities that might affect you.

  * Domain ontologies: it is now possible to create domain-specific SPARQL
    endpoints with customizable ontologies and data locations. It is possible
    to do so either in-process using the traditional Tracker daemons to
    do the work (Note: miners rely on Nepomuk ontology).
  * Deleted nautilus/firefox/evolution/thunderbird plugins. They were in
    a pitiful state of maintainership, the latter 2 being known broken for
    some time already.
  * Deleted tracker-needle. It has been in strict maintenance state for years,
    since then other UIs have integrated Tracker search and offer a more
    compelling and modern experience.
  * Deleted tracker-preferences. Tracker has been using GSettings for years,
    so doesn't specifically require an UI for it, there's of course room
    for desktop integrated configuration, gnome-control-center does this
    for example.
  * Made portions of libtracker-miner useful on the client side. Miners don't
    mandate anymore an org.freedesktop.Tracker1.Miner DBus interface/name,
    this is instead delegated on TrackerMinerProxy. TrackerMinerFS has been
    made largely independent of the ontology in use, all high level operations
    that do require SPARQL updates are now exposed in TrackerMinerFSClass.
  * Removed deprecated and duplicate API on libtracker-miner
  * Added documentation about creating and maintaining ontologies.
  * Removed maemo leftovers.
  * libtracker-sparql: Protect sqlite3_bind* with interface lock
  * Improvements to meson build

Translations: ca, sr

NEW in 1.13.0 - 2017-05-23
==========================

  * Added support for the Meson build system, autotools will be eventually
    discontinued.
  * Compile fixes
  * Fixed introspection annotations
  * libtracker-sparql: Support osinfo ontology in TrackerNamespace
  * Fix possible source of stale files in TrackerFileSystem
  * Whitelist some more syscalls

Translations: fur, ru

NEW in 1.12.0 - 2017-03-20
==========================

  * Multiple compile warning fixes
  * Fix compilation on older vala

  Overview of changes between 1.10 and 1.12:

  * The extractors are now sandboxed
  * Small improvements towards full sparql 1.1 compliance
  * Many fixes for Coverity warnings
  * Thread contention in direct-access tracker clients has been
    eliminated, concurrent queries are now significantly faster.
  * Many small fixes all over the place.

Translations: da, id, it, ko, lt, lv

NEW in 1.11.4 - 2017-02-23
==========================

  * tracker-extract: More sandboxing stability fixes, more whitelisted
    inocuous syscalls. Compile fixes for any older kernel not providing
    any of the specified syscalls. Fixed dconf warnings when it accidentally
    hit the sandbox.
  * libtracker-sparql: Fixed date format generated by TrackerResource
  * drop libgee dependency
  * Coverity fixes all over the place

Translations: da, de, eu, fi, fr, gl, nb, pl, sk, sr, sr@latin, sv, zh_TW

NEW in 1.11.3 - 2017-01-02
==========================

  * tracker-extract: Whitelist multiple inocuous syscalls that were
    reported to raise false positives in the extraction sandbox.
  * Make libseccomp dependency only mandatory on Linux
  * Fix several leaks and Coverity warnings
  * Fixed tracker-extract insertion of pre-defined resources
  * Fixed TrackerResource SPARQL generation of rdfs:Resource properties
    with cardinality>1

NEW in 1.11.2 - 2016-12-08
==========================

  * tracker-extract: Sandbox extractor threads. Filesystem and network
    access are limited to being read and local only.
  * tracker-miner-fs: Fixed high CPU use when receiving many writeback
    notifications at once.
  * tracker-extract, libtracker-sparql, libtracker-miner: plug leaks
  * tests: cleanups and improvements

Translations: hu

NEW in 1.11.1 - 2016-11-21
==========================

  Brown paper bag release, revert BIND() fix as it breaks other legit
  cases.

NEW in 1.11.0 - 2016-11-21
==========================

  * libtracker-sparql: Added TrackerNotifier, helper object to receive
    notifications of changes to the Tracker database. All users of the
    GraphUpdated DBus signal are recommended to switch to it.
  * libtracker-sparql: Added client-side support for HTTP SPARQL
    endpoints.
  * libtracker-direct: Much reduced mutex contention during threaded/async
    queries on the direct access backend.
  * libtracker-sparql: Using BIND() after OPTIONAL{} now works properly
  * tracker-extract: Many improvements to music extraction, better labeling
    of albums, nmm:albumArtist metadata is more faithful to the file
    metadata.
  * libtracker-data: Fixed possible overflows in tracker:normalize/unaccent
  * Other fixes and cleanups.

Translations: ca, cs, de, es, eu, hu, lt, fur, pl, sv, zh_CN

NEW in 1.10.0 - 2016-09-19
=========================

Translations: da, el, en_GB

NEW in 1.9.2 - 2016-09-14
=========================

  * Restore trailing colon in nfo:Equipment URIs
  * Add new mime-types for comic books

Translations: da, fur, fr, gl, kk, ko, lv, pl, pt_BR, sr, sr@latin, sv, vi

NEW in 1.9.1 - 2016-08-23
=========================

  * Tracker-resource: New API to describe RDF resources that can be serialized
      into SPARQL updates and various data formats. TrackerSparqlBuilder will
      be eventually phased out by this API.
  * Tracker-extract:
    * Use tracker resource integrally.
    * Fixed blacklisting of crashy files.
    * Fixes in gstreamer module for 32-bit platforms
  * Libtracker-control:
    * Expose "index for process" miner API
  * Command line tools:
    * Add "tracker extract" subcommand
  * SPARQL:
    * Accept INSERT DATA, DELETE DATA and DELETE WHERE syntax again.
  * Libtracker miner: Fix accounting in TrackerPriorityQueue when removing
       elements.

Translations: cs, de, es, fr, hu, id, lt, pl, pt, sk

NEW in 1.9.0 - 2016-06-21
=========================

  * Adapt to new GKqueue monitor mapping.
  * Remove build time dependency on gnome-common
  * Fix error handling of tracker-extract-persistence
  * Fix tracker-miner-fs to honor all configuration options at runtime.
  * Stop recommending hard resets all through
    * tracker reset -r will now warn and request the user to explicitly allow
      the operation.
    * tracker-preferences won't show anymore a big "reset and restart" button.
  * Added "tracker reset -f $filename" subcommand. This will recursively reset
    all indexed content for the given filename/uri, and trigger reindexing if
    appropriate, so contents are just like freshly indexed.
  * Fixed possible crash in MP3 extractor
  * Favor embedded/external cue sheets before flac files' TOC.
  * store albumArtist from TPE2 tag in MP3 extractor
  * Avoid possible integer overflow in GIF extractor
  * Support regular expressions for fn:replace
  * Mark most internal functions as SQLITE_DETERMINISTIC
  * Logging changes in tracker-miner-fs, sparql errors no longer end up with
    full insert queries being logged, but a loud warning with instructions to
    get more info will be printed instead.
  * Fix FS size calculations on OpenBSD
  * Add MS Office "owner files" to ignored-files
  * Add systemd user services corresponding to D-Bus session services
  * Handle DjVu files
  * Fixes in handling of BIND()
  * miner-fs: Fix handling files moved soon after creating
  * Improved console output of tracker subcommands

Translations: de, es, oc, pt, pt_BR, sk

NEW in 1.8.0 - 2016-03-21
=========================

Changes since 1.7.5:
  * libtracker-miner: Adapt to libcue 2.0
  * translations: da

Overview of changes since 1.6.x:
  * Better support of the Sparql 1.1 syntax:
    - Support for many builtin functions:
      * String: CONCAT, CONTAINS, LCASE, UCASE, STRLEN, SUBSTR, STRSTARTS, STRENDS,
        STRBEFORE, STRAFTER, ENCODE_FOR_URI.
      * Numeric: ABS, ROUND, CEIL, FLOOR, RAND.
      * Date/Time: YEAR, MONTH, DAY, HOUR, MINUTES, SECONDS, NOW.
      * Checksum: MD5, SHA1, SHA256, SHA512.
  * Support for Sparql 1.1's DELETE {} INSERT {} WHERE {} syntax. This allows for
    performing complex updates in a single operation, without the shortcomings of
    the tracker specific INSERT OR REPLACE {} syntax.
  * Support for Sparql 1.1's BIND(). This allows mapping complex evaluations to a
    variable within the triple pattern, so usable within it.
  * The latter two applied allowed tracker-miner-fs to halve the time spent in
    handling move operations.
  * Many fixes to tracker-miner-fs and libtracker-miner, it is now a lot more robust
    esp. around cancellation situations (eg. deleting/unmounting the folder currently
    being indexed).
  * Many fixes to tracker-extract modules, many consistency fixes have been fixed where
    modules would produce invalid Sparql (eg. breaking cardinality limits of certain
    variables) if given files with unexpected metadata.
  * As a result of the last two points, 1.8.0 fixes most of the warnings/criticals that
    have been reported as "spamming journald".
  * Tracker now uses GTask, except for vala code.

NEW in 1.7.5 - 2016-03-14
=========================

  * Add back .trackerignore match to ignored-directories-with-content
  * tracker-extract: Fix gstreamer module cuesheet handling
  * libtracker-miner: Avoid querying file type in crawling queries
  * libtracker-data: Handle inserts where the subproperty cardinality is larger than the parents'
  * tracker-extract: Protect all single-valued properties in abiword extractor
  * tracker-extract: Protect all single-valued properties in EPUB extractor
  * tracker-extract: Protect all single-valued properties in ooxml extractor
  * tracker-extract: Protect all single-valued properties in oasis extractor
  * tracker-extract: Protect all single-valued properties in HTML extractor
  * tracker-extract: Check string length before parsing XMP in PDF extractor
  * tracker-extract: Add missing application/msword mimetype
  * libtracker-miner: Cut some slack on the reentry counter
  * libtracker-miner: Avoid changing order of elements in processing queue
  * libtracker-extract: Delete TrackerExtractClient
  * tracker-extract: Remove old dbus interface xml
  * tracker-extract: propagate urn to the TrackerExtractInfo
  * tracker-extract: Use safer method to insert tags in PDF module
  * tracker-extract: Use safer method to insert tags in GIF module
  * tracker-extract: Use safer method to insert tags in JPEG module
  * tracker-extract: Use safer method to insert tags in TIFF module
  * tracker-extract: Use safer method to insert tags in PNG module
  * libtracker-miner: Initialize all NodeData memory
  * libtracker-miner: Ensure the directory root is removed when its indexing root is

NEW in 1.7.4 - 2016-03-01
=========================

  * Update to FTS5.
  * libtracker-miner: Many fixes to TrackerFileNotifier cancellation
  * libtracker-direct: Handle cancellable argument in queries
  * libtracker-miner: Plug fd leak on TrackerCrawler cancellation
  * libtracker-extract: Fix year-only date extraction in gstreamer module
  * libtracker-extract: Use tracker-guarantee to ensure a title in playlists
  * tracker-miner-fs: Ignore git repositories. Modify the ignored-directories-with-content
      setting if you found this convenient.
  * tracker-miner-fs: Ignore #*# vim backups

Translations: oc

NEW in 1.7.3 - 2016-02-16
=========================

  * libtracker-miner: Many indexing fixes leading to stale elements in the database.
      If "UNIQUE constraint failed: nie:DataObject.nie:url" errors are seen in
      journald, running tracker-miner-fs once with the TRACKER_MINER_FORCE_CHECK_UPDATED
      envvar is recommended. you will need to terminate miners before that with
      tracker daemon -t
  * libtracker-miner: Do not insert partial/empty sparql on error
  * libtracker-miner: Pass a builder in UPDATE state to TrackerMinerFS::remove-file
  * libtracker-miner: Remove children recursively from queues on directory deleted
  * libtracker-miner: Fix generated Sparql query in sparql_contents_compose_query()
  * libtracker-miner: Fix some memory leaks of TrackerTask
  * libtracker-miner: Invalidate files iri recursively in case of file removal
  * libtracker-miner: Reset of reentry counter is not needed anymore
  * libtracker-fts: Fix invalid blob length calculation
  * libtracker-common: Use guint64 for free space calculations
  * libtracker-data, docs, libtracker-miner: Fix compile warnings
  * libtracker-data: misc code fixes
  * libtracker-data: Fix g_warning() missing argument
  * Update AppData to spec version 0.7+

Translations: lv

NEW in 1.7.2 - 2016-01-17
=========================

  * Many leak fixes.
  * libtracker-data: Reverted code to clean up stale Resources, can't just be done yet.
  * tracker tool: Removed tracker-compatibility CLI wrapper for older commands.
  * libtracker-common: Fix possible warnings on libicu unaccent code
  * ontology: Set domain index on nie:contentCreated for nmo:Message
  * libtracker-miner: Add ::remove-file signal vfunc
  * libtracker-common: Return total available space if running as admin.

Translations: lt

NEW in 1.7.1 - 2015-12-17
=========================

  * libtracker-data: Support for Sparql 1.1 BIND and
    DELETE {...} INSERT {...} WHERE {...} syntax.
  * libtracker-data: Support for Sparql 1.1 functions: NOW, RAND
  * libtracker-miner: Improved performance of move operations in TrackerMinerFS
  * libtracker-data: Silence criticals on REGEX() with empty strings
  * libtracker-miner, libtracker-extract: Fix double frees

Translations: sl, sv

NEW in 1.7.0 - 2015-11-25
=========================

  * libtracker-data: Support for Sparql 1.1 functions: CONCAT, CONTAINS, LCASE/UCASE, STRLEN,
    SUBSTR, STRSTARTS/STRENDS, ABS, ROUND, ENCODE_FOR_URI, STRBEFORE/STRAFTER, CEIL/FLOOR,
    YEAR/MONTH/DAY/HOUR/MINUTES/SECONDS, MD5/SHA1/SHA256/SHA512
  * libtracker-miner: Move previous data deletion on file updates to TrackerMinerFS implementations
  * libtracker-miner/libtracker-data/libtracker-extract: Partial port to GTask
  * tracker tool: Fixes to UID detection
  * libtracker-miner: Fix cancellation of tasks during TrackerFileNotifier destruction
  * libtracker-miner: Handle failure to get a TrackerSparqlConnection
  * libtracker-common: Fix buffer overrun in libunistring-based unaccenting
  * libtracker-control: Documentation fixes
  * tracker-extract: Photo orientation extraction fixes (TIFF, XMP)
  * Many fixes to functional tests

Translations: eu, it, sr, sr@latin, zh_CN

NEW in 1.6.0 - 2015-09-22
=========================

  * tracker-extract: Fix synchronization with tracker-miner-fs when wait-for-miner-fs=TRUE
  * tracker-miner-fs: Fix crash during startup
  * tracker-extract: Fix builtin dummy module struct

Translations: da, de, el, fr, gl, id, ko, pl, pt_BR, nb, sl, sv, tr, ru, zh_TW

NEW in 1.5.2 - 2015-08-20
=========================

  * libtracker-data: Fix printf string format
  * libtracker-miner: Fallback to basename checks on hidden files
  * rss: Set website url as a nfo:WebSite
  * rss: Simplify GrssFeedChannel list creation
  * libtracker-data: Clean up stale URIs on startup
  * rss: Optimize deletes
  * rss: Perform extraction/insertion of feed items at once
  * ontology: Remove cardinality limits on nmo:communicationChannel
  * libtracker-common: String to date conversion to return with GError when null string
  * libtracker-extract: Add builtin dummy extractor
  * tracker-extract: Use dummy fastpath for svg extraction
  * libtracker-extract: Plug leaks
  * libtracker-miner: Cancellation on unmount fixes
  * libtracker-miner: Deprecate tracker_miner_fs_add_directory_without_parent
  * tracker-miner-fs: Keep cache of IndexFile requesters on directories

Translations: ca, cz, lt, pl, pt_BR, pt, sk, tr

NEW in 1.5.1 - 2015-07-21
=========================

  * Many fixes to RSS miner:
    - Dumps more complete data on tracker-store.
    - Stability fixes.
    - Leak fixes.
    - Performs automatic maintenance of feed messages.
  * Bumped libgrss dependency on 0.7
  * Performance improvements on tracker-store delete operations
  * Performance improvements on tracker-miner-fs delete operation handling.
  * Fix main Resource table id/urn leaks
  * Fix unnecessary queries in tracker-extract

Translations: es, hu, pt

NEW in 1.5.0 - 2015-07-13
=========================

  * Fix possible scenarios for high memory consumption in tracker-extract
  * Ontology documentation has been revamped
  * Add search by tags functionality to tracker-needle
  * Fix spurious "out of space" checks succeeding (#748907)
  * Fix tracker-miner-fs not entering idle state (#751172)
  * File monitoring fixes (#750394)
  * MP3 extraction fixes (#733136, #728252, #735515)
  * EPUB extraction fixes (#746401)
  * Fixes on gstreamer-based album extraction (#748227, #732236)
  * GSF-based extraction fixes (#730085, #746437)
  * Fix various compiler warnings (#746381, #746194, #751051)
  * OpenBSD portability fixes (#697719)
  * Made tracker-needle use gsettings (#740302)
  * Added infrastructure to rebuild FTS tokens on tokenizer changes
  * Fixed first index of gnome user docs (#751992)
  * Allow dummy .rule files for extractors (#735610)

Translations: bg, ca, cs, de, es, hu, id, it, pl, pt, pt_BR, sk, sl, sv, tr, zh_TW


NEW in 1.3.6 - 2015-03-17
=========================

  * Fix spurious folder deletes/reindexes (#741852)
  * Fix nie:url UNIQUE constraint asserts on downloaded files (RH#1192224)
  * Clear tracker-store watchdog timeout (#745565)
  * Support fn:replace (#745917)
  * Spam stderr less for not-so-uncommon error conditions (#746256)

Translations: ko, bs, sr, sl, da


NEW in 1.3.5 - 2015-03-06
=========================

  * Fix major database migration bug, skip Tracker 1.3.4. (#745737)
  * Build only libiptc test if libjpeg is enabled (#745583)
  * Put absolute path in shell script (#743738)

Translations: ru


NEW in 1.3.4 - 2015-03-03
=========================

  * Fixed playlists being processed by tracker-extract
  * Fixes: #743727, Account for cardinality=0 on DB migration
  * Fixes: #745219, mp3 passes album instead of song title to libmediaart

Translations: zh_TW, sk, eo, nb, lt, eu, po


NEW in 1.3.3 - 2015-02-05
=========================

  * Fixes: GB#681698, Disc number may be incorrect for multi-disc vorbis / flac albums
  * Fixes: GB#741109, BMP extractor doesn't retrieve image height and width
  * Fixes: GB#743250, libmediart-2.0 support
  * Fixes: GB#735406, Tracker spams my log with "Unable to insert multiple values for subject ..."
  * Fixes: GB#742391, applications, userguides: Don't detect locale changes if we failed to create the miner
  * Fixes: GB#742035, 1.2.5 unit test failures
  * Fixes: GB#739234, Tracker sometimes ignores SIGINT/SIGTERM
  * Fixes: GB#663687, Missing accessible label names on tracker-needle widgets
  * Fixes: GB#741777, libtracker-sparql: Document requirement to escape constructed queries
  * Fixes: GB#678986, item_move() creates invalid SPARQL if source file does not exist
  * Reverted: GB#733536, Remove --enable-guarantee metadata, always guarantee nie:title and nie:contentCreated

Translations:

  * Updated cs: Marek Černocký
  * Updated de: Christian Kirbach, Bernd Homuth
  * Updated el: Dimitris Spingos, Dimitris Spingos (Δημήτρης Σπίγγος)
  * Updated es: Daniel Mustieles
  * Updated eu: Inaki Larranaga Murgoitio, Iñaki Larrañaga Murgoitio
  * Updated gl: Fran Dieguez
  * Updated hu: Balázs Úr
  * Updated pt_BR: Rafael Ferreira


NEW in 1.3.2 - 2014-12-11
=========================

  * New Feature: 'tracker' binary: All pre-existing tracker-* command line tools have been merged into this new binary. Man pages are shown for 'help' sub-commands. This brings more of a "git" feel to the tracker's command line tools. There is a backwards compatible 'tracker-command' script and compatibility layer provided to ensure existing commands still work for a limited time. The old 'tracker-control' actions are now part of 'tracker daemon', 'tracker reset' and 'tracker index'. Additionally, the 'tracker-sql' utility has been integrated to allow SQL operations directly on the DB.
  * Fixes: GB#741317, miner-fs: use GFile to check for homedir
  * Fixes: GB#741141, Tracker crashes every 5-10 minutes
  * Fixes: RH#1133042, [abrt] tracker: persistence_store_file(): tracker-extract killed by SIGSEGV
  * all: Move all code from tracker_info() to g_info() new in GLib 2.40.
  * gvdb: Fixed LDFLAGS and CFLAGS, was using unused variables
  * gvdb: Move away from g_file_delete() to unlink() to avoid GIO dep for one line in unit tests
  * libtracker-fts: Make property list per-thread data to fix uninitialisation issues
  * libtracker-fts: Fixed compilation errors when --disable-tracker-fts is used
  * tracker-extract: Fixed potential use of uninitialised hash table in persistence_invalidate()
  * tracker-extract: Fixed possible invalid memory read/write in tracker_keywords_parse()
  * tracker-extract: Fixed memory leak of MediaArt object for TrackerExtractInfo
  * clean-tracker-prefix: Should consider lib64 as well as lib directories

Translations:

  * Updated pl: Piotr Drąg


NEW in 1.3.1 - 2014-12-03
=========================

  * Fixes: GB#733536, Remove --enable-guarantee metadata, always guarantee nie:title and nie:contentCreated
  * Fixes: GB#739260, srcdir != builddir issue with latest git
  * Fixes: GB#741012, should not override "all" target
  * Fixes: GB#740268, System tab - very ambiguous about what is being removed
  * Fixes: GB#740864, Symlink target for D-Bus service files includes DESTDIR

Translations:

  * Updated cs: Marek Černocký
  * Updated es: Daniel Mustieles
  * Updated hu: Balázs Úr
  * Updated nb: Kjartan Maraas
  * Updated pt: Duarte Loreto, Pedro Albuquerque


NEW in 1.3.0 - 2014-11-06
=========================

The main changes since 1.2.x are:

  * build: Require GLib >= 2.4.0, for g_str_match_string() and g_info()
  * all: Migrate to GKeyfileSettingsBackend, including:
   * Now when using TRACKER_USE_CONFIG_FILES env var, we use this backend
   * MAN pages which previously didn't exist for configurations are generated from xml gschemas
  * libstemmer: Removed imported code and depend on external version
  * libtracker-miner: Major update to Vala bindings
  * libtracker-common: Major clean up and removal of unused internal APIs, including:
    * Removed os-dependent APIs (mainly process spawning GLib does now anyway)
    * Removed tracker_file_(un)lock() API which was unused
    * Removed all run-time locale management, GNOME handles this between logins, so do we
    * Removed TrackerConfigFile and TrackerKeyfileObject classes, use GSettings instead
    * Removed unused type utils functions (e.g. tracker_*_to_string() and more)
    * Moved tracker-ontologies.h (with prefix, etc definitions) into libtracker-sparql
    * Moved CRC32 API into libtracker-data (the only place it's used)
    * Moved TrackerStorage class to tracker-miner-fs (only place it's used)
  * data: Now moved into their respective binary dirs, including:
    * .desktop: Desktop files (moved from data/ into src/tracker-store, etc)
    * .pc: Pkgconfig files (moved from data/ into src/libtracker-miner, etc)
    * .service: DBus files (moved from data/dbus/* to src/tracker-store, etc)
    * .png, .svg: Moved artwork into src/tracker-needle)
    * .gschema.xml: GSettings files (moved from data/gschemas into src, tracker-store, etc)
    * .ontology: Ontologies are now in src/ontologies
    * stopwords.*: Stop words per language (moved from data/stopwords into src/libtracker-common for TrackerLanguage)
  * data: Merged all data miner .desktop files with existing DBus .service files
  * functional-tests: Many updates to fix broken functional tests, including:
    * Writeback and miner base class clean ups
    * Removing IgnoreNextUpdate wrapper and 'wait for miner to be idle' calls
    * Waiting properly for resource addition / removal
  * Fixes GB#737663, Avoid the use of setrlimit() causing SIGABRT in tracker-extract

Translations:

  * Updated fi: Timo Jyrinki, Jiri Grönroos
  * Updated hu: Gábor Kelemen, Gabor Kelemen
  * Updated it: Milo Casagrande


NEW in 1.2.0 - 2014-09-22
=========================

The main changes since 1.0.x are:

  * New Feature: Extractor now supports eBook and Comic book formats
  * New Feature: Extractor now supports BMP files
  * New Feature: Data Provider: Now there are TrackerDataProvider and TrackerEnumerator interfaces to allow 3rd parties to supply their own data to Tracker to be indexed, for example for an online or proprietary service
  * New Feature: Extractor is now 'crash-aware' that being, we have a separate data source for content that can't be extracted to avoid repeated failures from tracker-extract. This also survives through crashes that may occur due to attempted extractions
  * build: Updated doap file (added programming language and category is now "core")
  * build: Improve firefox and thunderbird detection to avoid warnings in console
  * build: Removed all GNU_SOURCE additions in sources, use AC_USE_SYSTEM_EXTENSIONS instead
  * build: Fixed many unit tests that were not properly isolated
  * build: configure now has an --enable-minimal option to ONLY build tracker-store
  * build: Reduce vala generated source warnings from GCC
  * build: Use major version only PkgConfig files. Previously, we had $library-$major-$minor (e.g. tracker-sparql-1.2, when it should be tracker-sparql-1.0) to avoid creating unnecessary work for developers using Tracker. This release corrects this and it was planned originally.
  * build: Include COPYING*GPL files in tarball
  * build: Generate Vala bindings from GIR for libtracker-miner, libtracker-control
  * build: Improved many GIR annotations for nullable cases in libtracker-miner, libtracker-control
  * build: Avoid using rpath where possible
  * build: Make check fails to generate test reports
  * build: Remove --enable-guarantee metadata, always guarantee nie:title and nie:contentCreated
  * build: Added CC_CHECK_FLAGS_APPEND m4 macros from xinelib/systemd projects to check GCC compiler flags we can use
  * ontology: nfo:hasMediaStream no longer has a max cardinality of 1, this may mean a reindex is needed
  * libtracker-bus: Fixed a crash in query_async() where splice async operation is cancelled
  * libtracker-bus: Fixed a bug where tracker-stats showed no information with Vala 0.24+
  * libtracker-sparql: All TrackerSparqlConnection APIs throw GLib.Error due to GUnixFDList _append() API
  * libtracker-sparql: Move TRACKER_ const definitions to Tracker namespace, i.e. Tracker.DBUS_*
  * libtracker-miner: Fixed reference leak with TrackerTaskPool when using external data miners
  * libtracker-miner: Add new API tracker_decorator_fs_prepend_file() as part of 'crash-aware' feature.
  * libtracker-miner: PkgConfig file now requires libmediaart (when dependent) privately
  * libtracker-extract: Fixed broken EXIF GPS coordinate extraction
  * libtracker-extract: There is no longer an example in the examples/ directory as this library is now private
  * tracker-extract: Fixed missing TRACKER_EXTRACTORS_DIR env var that was removed at some stage
  * tracker-extract: Cleaned up signal handling, SIGALRM and SIGABRT no longer used
  * tracker-extract: [DOC] Catch more MS Office MIME types supported
  * tracker-extract: [MP3] Fixed mp3 parsing for ID3v24 or ID3v23 tags
  * tracker-extract: [ISO] Extract language, copyright and author information about ISO images using new ontology
  * tracker-extract: [ODT] Improved extraction of content so we don't stop on embedded tabs and line breaks
  * tracker-extract: [GIF] Support giflib 5.1.0 as well as earlier versions
  * tracker-stats: Now EXPRESSIONS can be used to filter stats shown
  * tracker-stats: Now only common and fallback RDF types are shown by default, --all can be used to old behaviour
  * tracker-info: Add --resource-is-iri or -i to avoid filename to URN lookups
  * tracker-info: Don't show "(null)" for nie:InformationElements with no nie:url
  * tracker-control: Added a lot of strings previsouly untranslated
  * tracker-control: Removed shorthand command line switches for less common arguments (--list-*, --pause-details)
  * tracker-control: Added --watch command line option to watch changes to the database.
  * tracker-miner-fs: By default, ignore directories with '.trackerignore' file inside
  * tracker-miner-apps: New process to mine data about applications, previously part of tracker-miner-fs. Can be disabled with --disable-miner-apps
  * tracker-miner-user-guides: New process to mine data about user guides and help content, previously part of tracker-miner-fs. Can be disabled with --disable-user-guides
  * tracker-writeback: Support GIFs
  * tracker-preferences: Removed fixed column for indexed locations, column header text was cut off
  * tracker-preferences: Don't use Stock.* deprecated since Gtk+ 3.10
  * tracker-preferences: Added suggestion to reindex OR restart depending on configuration changes applied
  * tracker-preferences: General UI clean up and modernization fixing many deprecations where old GTK+ widgets were used
  * tracker-preferences: Added 'control' tab to allow users to enable finer control of what's indexed (e.g. numbers - which are normally disabled)
  * tracker-preferences: Add AppData with screenshots for app stores
  * tracker-needle: Add AppData with screenshots for app stores
  * tracker-sandbox: Setup xdg-user-dirs too

Bugs Fixed (31):

  https://bugzilla.gnome.org/buglist.cgi?quicksearch=737023,736328,735180,735645,735636,735460,735610,735567,735317,735339,733573,733317,734464,733857,734089,734144,733316,733948,733863,733536,733439,732839,730627,728370,730111,729253,729968,729708,728252,728546,725717

Translations:

  Many, thank you to everyone involved.


NEW in 1.1.4 - 2014-09-05
=========================

  * New Feature: Extractor now supports eBook and Comic book formats
  * Fixes: GB#735645, EPub extractor bug fixes
  * Fixes: GB#735636, tracker-sandbox: Setup xdg-user-dirs
  * Fixes: GB#735460, ePub/eBooks indexing bugs
  * Fixes: GB#735610, Remove requirement on ModulePath for extractor .rule files
  * Fixes: GB#735567, build: include COPYING*GPL files in tarball
  * Fixes: GB#735317, Tracker no longer honours my 'ignore files' put on the disks
  * Fixes: GB#735339, undefined symbol: tracker_enumerator_get_type
  * Fixes: GB#733573, playlists are given RDF type nmm:MusicPiece and nfo:Audio
  * Fixes: GB#733317, tracker-extract: remove application/vnd.ms-* catchall from msoffice
  * ontology: Added new RDF type nfo:EBook for electronic books, this is not in Nepomuk yet
  * libtracker-fts: Fixed a memory leak for property_names variable
  * libtracker-data: Fixed compiler warning with tracker_db_interface_sqlite_fts_update_text()
  * libtracker-bus: Fixed a bug where tracker-stats showed no information with Vala 0.24+
  * libtracker-bus: Fixed all TRACKER_DBUS* to Tracker.DBUS
  * libtracker-sparql: All TrackerSparqlConnection APIs throw GLib.Error due to GUnixFDList _append() API
  * libtracker-sparql: Move TRACKER_ const definitions to Tracker namespace, i.e. Tracker.DBUS_*
  * libtracker-miner: Generate Vala bindings from GIR
  * libtracker-miner: Improve GIR annotations for some nullable cases
  * libtracker-miner: Removed TrackerCrawlFlags and use TrackerDirectoryFlags instead
  * libtracker-miner: Call tracker_data_provider_end_async() when dir has been crawled
  * libtracker-miner: Fixed reference leak with TrackerTaskPool when using external data miners
  * libtracker-extract: Fixed broken EXIF GPS coordinate extraction
  * libtracker-control: Generate Vala bindings from GIR
  * libtracker-control: Improve GIR annotations for some nullable cases
  * tracker-stats: Now EXPRESSIONS can be used to filter stats shown
  * tracker-stats: Now only common and fallback RDF types are shown by default, --all can be used to old behaviour
  * tracker-extract: Improved error parsing and content extraction for EPub contents
  * tracker-extract: Now EPub files are marked as new RDF type nfo:EBook
  * tracker-miner-fs: By default, ignore directories with '.trackerignore' file inside
  * tracker-preferences: Removed fixed column for indexed locations, column header text was cut off
  * tracker-sandbox: Setup xdg-user-dirs too

Translations:

  * Updated as: Nilamdyuti Goswami
  * Updated cs: Marek Černocký
  * Updated el: Tom Tryfonidis
  * Updated es: Daniel Mustieles
  * Updated fr: Alexandre Franke, Alain Lojewski
  * Updated gl: Fran Diéguez, Fran Dieguez
  * Updated id: Andika Triwidada
  * Updated pt_BR: Enrico Nicoletto
  * Updated ru: Yuri Myasoedov
  * Updated zh_HK: Chao-Hsiung Liao
  * Updated zh_TW: Chao-Hsiung Liao


NEW in 1.1.3 - 2014-08-19
=========================

  * New Feature: Extractor is now 'crash-aware' that being, we have a
    separate data source for content that can't be extracted to avoid
    repeated failures from tracker-extract. This also survives through
    crashes that may occur due to attempted extractions.
  * build: Reduce vala generated source warnings from GCC.
  * libtracker-fts: Make private functions static
  * libtracker-miner: Make sure we return NULL on FileDataProvider
    begin() errors
  * libtracker-miner: Add new API tracker_decorator_fs_prepend_file()
    as part of 'crash-aware' feature.

Translations:

  * Updated el: Tom Tryfonidis
  * Updated es: Daniel Mustieles
  * Updated fi: Lasse Liehu, Jiri Grönroos
  * Updated lt: Aurimas Černius


NEW in 1.1.2 - 2014-08-12
=========================

The main changes since 1.1.1 are:

  * New Feature: Data Provider: Now there are TrackerDataProvider and
    TrackerEnumerator interfaces to allow 3rd parties to supply their own
    data to Tracker to be indexed, for example for an online or
    proprietary service.
  * Fixes: GB#734464, tracker-guarantee-tests failure
  * Fixes: GB#733857, rpath is being set on tracker libraries
  * Fixes: GB#734089, testsuite summaries fail to report results
  * Fixes: GB#734144, Tracker processes fail to respond to INT or TERM while initialising
  * Fixes: GB#733316, writeback: support saving metadata for GIFs
  * Fixes: GB#733948, src/libtracker-extract/tracker-extract-info.c: Non-void function should return a value
  * Fixes: GB#733863, 0.5 (commit 880d09fbff1f89f4ce5ed9ee5b1d23ccc6e20da2 ) breaks tracker compilation
  * Fixes: GB#733536, Remove --enable-guarantee metadata, always guarantee nie:title and nie:contentCreated
  * Fixes: GB#733439, tracker should include COPYING.LGPL
  * build: Updated doap file (added programming language and category is now "core")
  * build: Improve firefox and thunderbird detection to avoid warnings in console
  * build: Removed all GNU_SOURCE additions in sources, use AC_USE_SYSTEM_EXTENSIONS instead
  * build: Fixed many unit tests that were not properly isolated
  * build: configure now has an --enable-minimal option to ONLY build tracker-store.
  * ontology: nfo:hasMediaStream no longer has a max cardinality of 1, this may mean a reindex is needed
  * libtracker-miner: PkgConfig file now requires libmediaart (when dependent) privately
  * libtracker-miner: Many documentation updates and fixes
  * libtracker-extract: There is no longer an example in the examples/ directory as this library is now private
  * tracker-extract-mp3: Fixed mp3 parsing for ID3v24 or ID3v23 tags
  * tracker-extract: Fixed missing TRACKER_EXTRACTORS_DIR env var that was removed at some stage
  * tracker-extract: Cleaned up signal handling, SIGALRM and SIGABRT no longer used
  * tracker-extract: When built with libmediaart, requires version 0.5.0 or higher.
  * tracker-preferences: Don't use Stock.* deprecated since Gtk+ 3.10
  * tracker-preferences: Added suggestion to reindex OR restart depending on configuration changes applied
  * functional-tests: Improved and updated in places

Translations:

  * Updated el: MarMav, Maria Mavridou
  * Updated es: Daniel Mustieles
  * Updated lt: Aurimas Černius
  * Updated pt_BR: Rafael Ferreira, Enrico Nicoletto
  * Updated zh_CN: YunQiang Su, Yunqiang Su


NEW in 1.1.1 - 2014-07-10
=========================

The main changes since 1.1.0 are:

  * build: Use major version only PkgConfig files. Previously, we had
    $library-$major-$minor (e.g. tracker-sparql-1.2, when it should be
    tracker-sparql-1.0) to avoid creating unnecessary work for
    developers using Tracker. This release corrects this and it was
    planned originally.


NEW in 1.1.0 - 2014-07-10
=========================

The main changes since 1.0.0 are:

  * build: Added CC_CHECK_FLAGS_APPEND m4 macros from xinelib/systemd projects to check GCC compiler flags we can use
  * tracker-miner-apps: New process to mine data about applications, previously part of tracker-miner-fs. Can be disabled with --disable-miner-apps
  * tracker-miner-user-guides: New process to mine data about user guides and help content, previously part of tracker-miner-fs. Can be disabled with --disable-user-guides
  * tracker-extract: [BMP] Added an new extractor
  * tracker-extract: [ISO] Extract language, copyright and author information about ISO images using new ontology
  * tracker-extract: [ODT] Improved extraction of content so we don't stop on embedded tabs and line breaks
  * tracker-control: Added a lot of strings previsouly untranslated
  * tracker-control: Removed shorthand command line switches for less common arguments (--list-*, --pause-details)
  * tracker-control: Added --watch command line option to watch changes to the database.
  * tracker-info: Add --resource-is-iri or -i to avoid filename to URN lookups
  * tracker-info: Don't show "(null)" for nie:InformationElements with no nie:url
  * tracker-preferences: General UI clean up and modernization fixing many deprecations where old GTK+ widgets were used
  * tracker-preferences: Added 'control' tab to allow users to enable finer control of what's indexed (e.g. numbers - which are normally disabled)
  * tracker-preferences: Add AppData with screenshots for app stores
  * tracker-needle: Add AppData with screenshots for app stores

Translations:

  * Updated ca: Gil Forcada, Pau Iranzo
  * Updated cs: Marek Černocký
  * Updated de: Christian Kirbach, Benjamin Steinwender
  * Updated el: Dimitris Spingos, marablack3, Dimitris Spingos (Δημήτρης Σπίγγος)
  * Updated es: Daniel Mustieles
  * Updated eu: Inaki Larranaga Murgoitio, Iñaki Larrañaga Murgoitio
  * Updated gl: Fran Diéguez, Fran Dieguez
  * Updated it: Milo Casagrande
  * Updated ko: Seong-ho Cho
  * Updated lt: Aurimas Černius
  * Updated pl: Piotr Drąg
  * Updated ru: Yuri Myasoedov
  * Updated sr: Мирослав Николић
  * Updated sr@latin: Мирослав Николић, Miroslav Nikolić
  * Updated tr: Muhammet Kara
  * Updated uk: Daniel Korostil
  * Updated zh_CN: YunQiang Su, Yunqiang Su


NEW in 1.0.0 - 2014-03-24
=========================

The main changes since 0.16.x are:

  * build: Require libmediaart >= 0.1.0 (soft dependency), code has been exported from Tracker to this library
  * build: Require libicu >= 4.8.1.1, first version with pkg-config support, no version specified before
  * build: Require Vala >= 0.18.0 (was 0.16.0) to avoid Vala 0.16 g_dbus_message_get_body() binding bug
  * build: Require GLib >= 2.38 (was 2.35.1) after removing all deprecated g_test_trap_fork() calls for _subprocess()
  * build: Require libosinfo >= 0.2.9 (was 0.0.2)
  * build: Updated GSF dependency to >=1.14.24
  * build: Improve SQLite FTS feature checking for cross compilation
  * build: Improve description of build switches
  * build: Remove ALL GLib marshallers. We depend on GLib (>= 2.30) which depends on libffi enabling us to do this
  * build: Make sure --target-glib used with Vala is the correct GLib (was 2.34, should be $GLIB_REQUIRED), removes Vala g_type_init() warnings.
  * build: Do not maintain LT versioning for internal libraries libtracker-{common,data,extract}
  * build: GTester Makefile rules have been updated
  * build: Improve portability, avoid some GNUisms and #ifdef linux specific API
  * build: Improved portabiliy for OpenBSD and Solaris
  * build: Use UPower >=0.99 if available
  * all: Make GBusType run-time configurable with TRACKER_BUS_TYPE environment variable
  * New Feature: Passive extraction: Specific metadata extraction (e.g. image orientation) is done by listening for insert signals for resource metadata (e.g. file name, size, etc) and done passively rather than being required before inserting ANY information about a resource. This officially makes tracker-extract a miner of special sorts
  * New Feature: SPARQL functions tracker:normalize() and tracker:unaccent()
  * New Feature: libav extractor for extracting audio/video metadata
  * New Feature: tracker-store GraphUpdated signal delay now configurable: This signal is what's used to notify of resource changes in the database (e.g. new file added), previously it was always 1 second, now it can be configured
  * libtracker-common: Added strnlen() compatibility function
  * libtracker-data: Fix AS ?foo handling in FTS queries
  * libtracker-direct: Don't allow connection if current and db locale mismatch
  * libtracker-miner: Major API cleanups,
    - TrackerMinerWeb has been removed, replaced by TrackerMinerOnline
    - TrackerMinerManager has been splitted into a separate libtracker-control
    - TrackerCrawler, TrackerStorage and the thumbnailer/mediaart helper functions are no longer public.
  * libtracker-extract: This library has been made private. Implementing a TrackerDecorator is the recommended way to provide additional data.
  * libtracker-control: New library, containing TrackerMinerManager, it requires using tracker-control-1.0.pc
  * tracker-miner-fs: optional prioritization of folders has been reimplemented, used on removable volumes and DBus requests
  * tracker-extract: Prioritization of rdf:types has been implemented, so certain file types are extracted before others, on demand of applications.
  * tracker-extract: Removed support for libstreamanalyzer, xine and totem extractors, not been used or supported in a long time
  * tracker-extract: Removed support for obsolete GStreamer backends tagreadbin, decodebin2 in gstreamer extractor
  * tracker-extract: Removed the need for fork() and sub-processes with PDF extraction, no longer needed
  * tracker-extract: Removed --disable-shutdown command line option now that it is instantiated by DBus
  * tracker-extract: Fixed epub extractor so it doesn't use a broken ontology
  * tracker-extract: Make it clearer where MIME type came from in debug (from GIO/xdgmime or tracker-miner-fs)
  * tracker-extract: EPub extractor has been improved, content is now more complete.
  * tracker-extract: Added ICU-based encoding detection
  * tracker-extract: Detect GNOME screenshots and these to a category
  * tracker-sparql: Now has a --tree switch to print ontology classes and properties as a tree
  * tracker-tag: Fixed GLib-WARNING **: goption.c:2193: ignoring no-arg...
  * tracker-tag: Fix --list (-t) to use logical OR, not AND
  * tracker-needle: always show "Loading..." in the Document category
  * tracker-sandbox: Added utility tracker-sandbox to run tracker in isolated cases
  * docs/tools/ttl2sgml: Don't GError on nao:deprecated namespaces in the ontology, errors for mlo which is entirely deprecated
  * functional-tests: Use GraphUpdated signal from store, not miner-fs Idle status
  * functional-tests: Don't require miner-fs being idle for testing data inserted
  * functional-tests: Start/stop processes in correct order, miner after extract
  * functional-tests: Increase default timeout 90s->180s, 90s isn't long enough sometimes
  * functional-tests: Don't rely on just tracker-control -t to kill tests in time
  * thunderbird: Added UI localization support including translations for French, Spanish and Polish
  * thunderbird: Escape folder names in URIs
  * thunderbird: Port code to make sure it works for Thunderbird 24

Bugs Fixed (29):

  https://bugzilla.gnome.org/buglist.cgi?quicksearch=705315,726419,726639,726640,725689,726552,726553,680897,726421,685378,725607,725890,724984,719802,723097,696099,721455,721880,722254,722525,701259,722353,720686,719512,715040,699412

  https://bugzilla.redhat.com/buglist.cgi?quicksearch=1026283%2C1034714

  https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=675198

Translations:

  Many, thank you to everyone involved.


NEW in 0.17.8 - 2014-03-21
==========================

The main changes are:

  * Fixes: GB#705315, tracker-0.16.2: configure wrongly mixes checks for enable-compile-warning while using WITH-compile-warnings
  * Fixes: GB#726419, tracker-extract reports sparql update errors for jpeg with dc:indenfitier
  * Fixes: DB#675198, tracker-store man page description needs minor cleanup

Translations:

  * Updated cs: Marek Černocký
  * Updated da: Ask H. Larsen
  * Updated hu: Gábor Kelemen
  * Updated id: Andika Triwidada
  * Updated lv: Rūdolfs Mazurs


NEW in 0.17.7 - 2014-03-19
==========================

The main changes are:

  * libtracker-sparql: Fix undefined reference to symbol 'tracker_ipc_bus'

Translations:

  * Updated it: Milo Casagrande
  * Updated sr: Мирослав Николић
  * Updated sr@latin: Мирослав Николић


NEW in 0.17.6 - 2014-03-18
==========================

The main changes are:

  * build: Improve SQLite FTS feature checking for cross compilation
  * build: Make libmediaart a soft dependency, not hard
  * build: Require libicu >= 4.8.1.1, first version with pkg-config support, no version specified before
  * Fixes GB#726421, fix crash in pdf extractor with parent/child process handling
  * Fixes GB#726639, Remove some obsolete extactors
  * Fixes GB#726640, tracker-extract: Add Gibest hash calculation to GStreamer
  * Fixes GB#725689, libtracker-extract: no longer use libtool versioning
  * Fixes GB#726552, Warning when running configure: "enable_tracker_miner_fs: command not found
  * Fixes GB#726553, Remove QT reference in configure.ac
  * Fixes GB#725607, Screenshots do not appear in Screenshots album
  * Fixes GB#725890, Miscellaneous memory fixes
  * all: Make GBusType run-time configurable with TRACKER_BUS_TYPE environment variable
  * all: Removed dead code from coverity report
  * libtracker-fts: Fix memory leak for GList on property names during FTS init
  * libtracker-data: getting FTS multivalued properties used unintialised variable in ontology_get_fts_properties()
  * libtracker-data: Protect potential NULL dereference in _db_cursor_iter_next()
  * libtracker-{common,data,extract}: do not maintain LT versioning
  * libtracker-miner: Do not leak the TrackerSparqlConnection in TrackerFileNotifier
  * libtracker-miner: Don't print uninitalised sparql variable for update_array_cb
  * libtracker-miner: Dispose GFileInfo objects ASAP when crawling (attempt to reduce memory footprint)
  * libtracker-extract: Lookup rules after initializing in get_fallback_rdf_types() to avoid empty rules lists
  * libtracker-extract: Fix memory leak on .rule file load errors
  * libtracker-extract: Fix memory leak and unref rdf_types hash table in _module_manager_get_fallback_rdf_types()
  * tracker-store: Remove the --low-memory option in the man pages, not used since 0.9.x
  * tracker-miner-fs: Fix memory leak on rdf_types GStrv in _files_add_rdf_types()
  * tracker-miner-fs: Fix memory leak on priv->compisite_device for UpDevice on _finalize()
  * tracker-extract: Removed support for libstreamanalyzer, xine and totem extractors, not been used or supported in a long time
  * tracker-extract: Removed support for obsolete GStreamer backends tagreadbin, decodebin2 in gstreamer extractor
  * tracker-extract: Don't report < 0 nfo:width for images with GStreamer discoverer
  * tracker-extract: Remove the need for fork() and sub-processes with PDF extraction, no longer needed
  * tracker-extract: Make logging clearer if we fail on no modules or no metadata
  * tracker-extract: Fixed epub extractor so it doesn't use a broken ontology
  * tracker-extract: Improve logging, too many g_debug() calls to other log levels
  * tracker-extract: set nfo:Video on the gstreamer guess rule for completeness
  * tracker-extract: Add desktop file to autostart process
  * tracker-extract: Make it clearer where MIME type came from in debug (from GIO/xdgmime or tracker-miner-fs)
  * tracker-sparql: Support listing properties with --tree and make --search case insensitive
  * tracker-sandbox: Set the locale properly so queries don't fail due to locale mismatch to DB

Translations:

  * Updated cs: Marek Černocký
  * Updated es: Daniel Mustieles
  * Updated fr: AlainLojewski, Claude Paroz, Luis Menina
  * Updated gl: Fran Dieguez, Fran Diéguez
  * Updated it: Milo Casagrande
  * Updated ko: Seong-ho Cho
  * Updated lt: Aurimas Černius
  * Updated lv: Rūdolfs Mazurs
  * Updated pl: Piotr Drąg
  * Updated pt_BR: Antonio Fernandes C. Neto, Rafael Ferreira
  * Updated ru: Yuri Myasoedov
  * Updated zh_CN: Wylmer Wang
  * Updated zh_HK: Chao-Hsiung Liao
  * Updated zh_TW: Chao-Hsiung Liao


NEW in 0.17.5 - 2014-03-04
==========================

The main changes are:

  * build: Improve description of build switches
  * build: Removed configure args no longer used for GdkPixbuf/Qt, Nemo and libsecret/gnome-keyring
  * build: Make libtracker-extract a private library
  * libtracker-miner: Possible use of uninitialized value in tracker_file_system_unset_property()
  * libtracker-miner: Unset priv->current_index_root when the TrackerFileNotifier is finished
  * libtracker-miner: Protect against NULL TrackerThumbnailer
  * libtracker-miner: clear thumbnailer internal objects on init() failure
  * libtracker-extract: Don't break compilation due to missing rules/modules dir
  * tracker-extract: Don't leak FD on tiff extractor when opening file
  * tracker-extract: set nfo:Document in FallbackRdfTypes for text documents
  * tracker-miner-fs: Fix documentation for 'low-disk-space-limit', was in MB not %
  * functional-tests: fix invalid SPARQL s/nao:hasTag:prefLabel/nao:prefLabel/
  * functional-tests: Use GraphUpdated signal from store, not miner-fs Idle status
  * functional-tests: Don't require miner-fs being idle for testing data inserted
  * functional-tests: 400-extractor.py expects different arguments
  * functional-tests: Start/stop processes in correct order, miner after extract
  * functional-tests: Increase default timeout 90s->180s, 90s isn't long enough sometimes
  * functional-tests: Don't rely on just tracker-control -t to kill tests in time
  * functional-tests: Fix regression introduced in commit 690eecb1

Translations:

  * Updated cs: Marek Černocký
  * Updated es: Daniel Mustieles
  * Updated gl: Fran Diéguez
  * Updated lt: Aurimas Černius
  * Updated pl: Piotr Drąg
  * Updated pt_BR: Rafael Ferreira
  * Updated zh_CN: Wylmer Wang


NEW in 0.17.4 - 2014-02-21
==========================

The main changes are:

  * rss: compile fix with the RSS miner enabled

Translations:

  * Updated br: Rafael Ferreira
  * Updated lt: Aurimas Černius

NEW in 0.17.3 - 2014-02-20
==========================

The main changes are:

  * tracker-extract: Prioritization of rdf:types has been implemented, so certain file types are extracted before others, on demand of applications.
  * libtracker-extract: This library has been made private. Implementing a TrackerDecorator is the recommended way to provide additional data.
  * libtracker-miner: Major API cleanups,
    - TrackerMinerWeb has been removed, replaced by TrackerMinerOnline
    - TrackerMinerManager has been splitted into a separate libtracker-control
    - TrackerCrawler, TrackerStorage and the thumbnailer/mediaart helper functions are no longer public.
  * libtracker-control: New library, containing TrackerMinerManager, it requires using tracker-control-1.0.pc
  * tracker-miner-fs: optional prioritization of folders has been reimplemented, used on removable volumes and DBus requests
  * tests: GTester Makefile rules have been updated
  * utils: tracker-sparql gained a --tree switch to print (partial) ontology trees, and longhands/shorthands for classes/properties


NEW in 0.17.2 - 2014-02-14
==========================

The main changes are:

  * build: Use API version 1.0 instead of bumping it every stable release (affects .pc and .so filenames)
  * build: Require libmediaart >= 0.1.0, code has been exported from Tracker to this library
  * build: Make sure --target-glib used with Vala is the correct GLib (was 2.34, should be $GLIB_REQUIRED), removes Vala g_type_init() warnings.
  * Fixes GB#696099, tracker needle always show "Loading..." in the Document category
  * Fixes GB#723097, Select search results when pressing down
  * Fixes GB#721455, Obsolete info in license text on multiple modules
  * libtracker-common: Add TRACKER_TRACKER_PREFIX define for tracker: ontology prefix
  * libtracker-miner: Ensure the decorator nie:DataSource is queried as an IRI
  * libtracker-miner: Make sure we flush remaining TrackerDecorator SPARQL updates when finished, avoids loosing extracted data for last files
  * libtracker-miner: Fix infinite loop TrackerDecorator
  * libtracker-miner: Fix warning for TrackerFileSystem registering properties twice (with hash destroy function set to NULL)
  * libtracker-miner: Improve TrackerDecoratorInfo to be a boxed type fixing a ref-count issue and a requirement for introspection.
  * libtracker-miner: Avoid unnecessary query calls when tracker_decorator_next() is called consecutively, we now queue queries.
  * libtracker-miner: Various improvements including moving string handling from GArray to GPtrArray and code duplication in TrackerDecorator.
  * tracker-extract: Split generic gstreamer/libav extract rules
  * tracker-extract: Use fully expanded name for nie:DataSource in decorator to avoid conflicts with shorthand insert IDs
  * tracker-needle: Fixed GTK+ 3.x deprecations
  * tracker-needle: Fixed typo in Makefile.am, VALACFLAGS was not being used, avoids warnings
  * firefox: Added Polish translations for plugin
  * thunderbird: Added UI localization support including translations for French, Spanish and Polish
  * thunderbird: Escape folder names in URIs
  * thunderbird: Port code to make sure it works for Thunderbird 24
  * thunderbird: Add and improve debug messages

Translations:

  * Updated cs: Marek Černocký
  * Updated gl: Fran Diéguez
  * Updated id: Andika Triwidada
  * Updated it: Milo Casagrande
  * Updated pt_BR: Rafael Ferreira
  * Updated zh_HK: Chao-Hsiung Liao
  * Updated zh_TW: Chao-Hsiung Liao


NEW in 0.17.1 - 2014-01-24
==========================

The main changes are:

  * build: Require Vala >= 0.18.0 (was 0.16.0) to avoid Vala 0.16 g_dbus_message_get_body() binding bug
  * build: Require GLib >= 2.38 (was 2.35.1) after removing all deprecated g_test_trap_fork() calls for _subprocess()
  * build: Require libosinfo >= 0.2.9 (was 0.0.2)
  * build: Use gtkdocize in autogen.sh differently to avoid "-chmod command not found", see GB#701259
  * build: Remove ALL GLib marshallers. We depend on GLib (>= 2.30) which depends on libffi enabling us to do this
  * build: Add SQLite 3.8.1 to list of bad SQLite versions
  * New Feature: Passive extraction:
                 Specific metadata extraction (e.g. image orientation)
                 is done by listening for insert signals for resource metadata
                 (e.g. file name, size, etc) and done passively rather than being
                 required before inserting ANY information about a resource
                 This officially makes tracker-extract a miner of special sorts
  * New Feature: SPARQL functions tracker:normalize() and tracker:unaccent()
  * New Feature: libav extractor for extracting audio/video metadata
  * New Feature: tracker-store GraphUpdated signal delay now configurable:
                 This signal is what's used to notify of resource changes in the database (e.g. new file added),
                 Previously it was always 1 second, now it can be configured
  * Fixes: GB#721880, fts:offsets returns wrong values
  * Fixes: GB#722525, #include <glib.h> is missing in src/tracker-ioprio.h
  * Fixes: GB#722353, #!/bin/bash in docs/tools/gen-doc.sh
  * Fixes: GB#720686, libtracker-extract: Link against libicu when using it
  * Fixes: GB#719512, Adapt to libosinfo 0.2.9 & make use of variant API
  * Fixes: GB#715040, Avoid starting tracker-store twice
  * Fixes: GB#699412, libtracker-fts/parser/nwords_6 test fails
  * Fixes: RH#1026283, Nautilus eating 100% cpu
  * libtracker-common: Added strnlen() compatibility function
  * libtracker-common: Fixed unit test for SCHED_IDLE expecting SCHED_OTHER by default, but jhbuild uses SCHED_IDLE as standard.
  * libtracker-fts: Strengthen against SQLite failures. Ensure no infinite loos in sqlite3_step() and don't use g_once_init_enter() for init
  * tracker-extract: Support Qt5
  * tracker-extract: Removed --disable-shutdown command line option now that it is instantiated by DBus
  * tracker-extract: Fixed memory leak with strings processed detected as UTF16
  * tracker-extract: msoffice: Fixed memory leak, calling g_free() on wrong pointer
  * tracker-extract: mp3: Fixed memory leak, cases where "" was found in tags were not always freed
  * tracker-extract: gstreamer: Fixed memory leak on error for GstDiscovererInfo structs
  * tracker-extract: gif: Fixed memory leak on error for filename
  * functional-tests: Set up XDG data and cache directories like we do in utils/sandbox/
  * functional-tests: Avoid invalid calls to g_source_remove(), avoids: "GLib-CRITICAL **: Source ID 130 was not found when attempting to remove it"
  * utils/sandbox/tracker-sandbox: Use separate dirs for config and data because there may be file name conflicts
  * docs/tools/ttl2sgml: Don't GError on nao:deprecated namespaces in the ontology, errors for mlo which is entirely deprecated

Translations:

  * Updated cs: Marek Černocký
  * Updated de: Christian Kirbach
  * Updated el: Dimitris Spingos
  * Updated es: Daniel Mustieles
  * Updated gl: Fran Diéguez
  * Updated it: Milo Casagrande
  * Updated nl: Erwin Poeze
  * Updated pl: Piotr Drąg
  * Updated pt_BR: Enrico Nicoletto, Rafael Ferreira
  * Updated uk: Daniel Korostil
  * Updated zh_CN: Tong Hui, 甘露(Gan Lu)


NEW in 0.17.0 - 2013-11-22
==========================

The main changes since 0.16.x are:

  * tracker-tag: Fixed GLib-WARNING **: goption.c:2193: ignoring no-arg...
  * tracker-tag: Fix --list (-t) to use logical OR, not AND
  * sparql: Fix AS ?foo handling in FTS queries
  * portability: Avoid some GNUisms and #ifdef linux specific API
  * portability: OpenBSD improvements
  * portability: Solaris improvements
  * portability: kqueue fixes
  * build: Stopped using deprecated glib functions
  * build: Use UPower >=0.99 if available
  * build: Improve warnings generated from Vala generated C files
  * build: Add compile time options to enable/disable multiple extractors
  * build: GIO vapi was updated
  * build: Updated GSF dependency to >=1.14.24
  * extractors: EPUB extractor has been improved, content is now more complete.
  * extractors: Added ICU-based encoding detection
  * extractors: improved media metadata in mp3 and gstreamer extractors.
  * extractors: detect GNOME screenshots and these to a category
  * utils: Added tracker-sandbox
  * documentation: Improved utils manpages and document some envvars
  * miner-fs: Improvements in indexing of removable media
  * libtracker-sparql-backend: Fix connection singleton mutex issue
  * sparql: stop using deprecated sqlite count_changes PRAGMA
  * libtracker-bus: allow cancellation when iterating cursor
  * libtracker-direct: don't allow connection if current and db locale mismatch

Translations:

  Many, thank you to everyone involved.

NEW in 0.16.0 - 2013-03-18
==========================

The main changes since 0.14.x are:

  * Require: GStreamer >= 1.0 (was 0.10) to follow upstream releases
  * Require: libgrss >= 0.5 (was 0.3) to follow upstream releases
  * Require: GLib 2.35.1 for GTask API (was 2.28.0)
  * Require: SQLite 3.7.9 (was 3.7.0) for FTS4 support
  * Removed: Flickr miner, it will be imported to GNOME Photos later.
  * Removed: Dia dependency for producing documentation.
  * Removed: tracker-explorer (unused and unmaintained)
  * Removed: tracker-search-bar (unused, useless and unmaintained)
  * Removed: utils/playlists (replicates tracker-extract)
  * Removed: utils/lyrics (not working any more, we don't store lyrics either)
  * Removed: utils/webhistory (unused)
  * Removed: utils/data-generators/deprecated/ (unused)
  * Removed: utils/data-generators/barnum/ (unused)
  * New Feature: Support GKqueueDirectoryMonitor API on BSD systems
  * New Feature: Full Text Search (FTS) v4 support:
                 New SPARQL function fts:snippet(?u)
                 - Snippets provide context around a matched query.
                 New SPARQL function fts:offsets(?u):
                 - Offsets provide an index where a matched query is
                   found in context.
                   See tracker-needle for basic use.
  * Build: Switched out all uses of deprecated GStaticMutex for GMutex
  * Build: Remove all cases of g_type_init() now it's deprecated in GLib 2.35.x
  * Ontology: Deprecated tracker:fulltextNoLimit and removed use in all other ontologies
  * libtracker-fts: Removed tracker_fts_config_min_word_length() and config option
  * libtracker-common: Add replacement function for flock() on Solaris
  * libtracker-common: Don't log to file by default, only stdout/stderr. See TRACKER_USE_LOG_FILES env var for details.
  * libtracker-miner: Fixed nasty stack corruption with TrackerIndexingTree filter rules
  * libtracker-miner: Fixed vala bindings for TrackerMinerFS
  * libtracker-miner: Renamed API tracker_media_art_queue_removal() to _remove()
  * libtracker-miner: Renamed API tracker_media_art_execute_queue() to _queue_empty()
  * libtracker-miner: Dispose regular files from the filesystem cache on monitor events
  * tracker-control: Fixed --{get|set}-log-verbosity with g_settings_sync()
  * tracker-miner-fs, store: Add MeeGo keys to desktop files
  * tracker-search: Added snippets for result hits
  * tracker-search: Added colored output for result hits
  * Nautilus: Fixed build error when supplying nautilis extensions directory
  * Thunderbird: Fixed extension path in configure.ac and look for correct libraries in $prefix
  * Firefox: Fixed extension path in configure.ac and look for correct libraries in $prefix

Bugs Fixed (18):

  https://bugzilla.gnome.org/buglist.cgi?quicksearch=693889,694146,691847,692409,692603,691807,691804,679870,690170,695769,643388,643388,695444,693198,695061,695157,694783,660965,692210

Translations:

  Many, thank you to everyone involved.


NEW in 0.15.4 - 2013-03-12
==========================

The changes are:

  * build: Fixed various compiler warnings
  * build: Fixed various introspection warnings
  * build: Switched out all uses of deprecated GStaticMutex for GMutex
  * Fixes: GB#643388, Sqlite constraint violation

Translations:

  * Updated el: Dimitris Spingos

Notes:

  * The 0.15.3 tarball uploaded was done using the wrong version of
    libgee for building the C sources from Vala. This is detailed in
    these bug reports:

      Tracker should build against libgee 0.8:
      https://bugzilla.gnome.org/show_bug.cgi?id=691807#c4

      tracker 0.15.3 fails to build: g_type_init is deprecated
      https://bugzilla.gnome.org/show_bug.cgi?id=695470

    This release fixes the tarball released AND includes the above
    fixes. Apologies for any inconvenience.


NEW in 0.15.3 - 2013-03-08
==========================

The changes are:

  * build: Require GLib 2.35.1 for GTask API (was 2.28.0)
  * build: Remove all cases of g_type_init() now it's deprecated in GLib 2.35.x
  * Fixes: GB#693889, E: tracker no-return-in-nonvoid-function tracker-data-manager.c:3610
  * Fixes: GB#694146, org.freedesktop.Tracker1.Miner.Progress() - wrong number of arguments introspected
  * Fixes: GB#660965, 100% CPU-load / segfault processing png-file
  * Fixes: GB#693198, tracker-miner-fs does not listen for config changes
  * Fixes: GB#695061, tracker-miner-fs, store: Add Unity to desktop files
  * Fixes: GB#694783, libtracker-miner: don't crash when first indexing files out of /home
  * Fixes: GB#695157, libtracker-miner: add an async version of tracker_miner_manager_index_file()
  * libtracker-common: Don't log to file by default, only stdout/stderr. See TRACKER_USE_LOG_FILES env var for details.
  * libtracker-miner: Fixed nasty stack corruption with TrackerIndexingTree filter rules
  * libtracker-miner: Fixed vala bindings for TrackerMinerFS
  * tracker-extract-msoffice: Avoid deprecated gsf_msole_metadata_read() API
  * tracker-search: Added snippets for result hits
  * tracker-search: Added colored output for result hits

Translations:

  * Updated cs: Marek Černocký
  * Updated de: Christian Kirbach
  * Updated es: Daniel Mustieles
  * Updated fr: Alexandre Franke
  * Updated pl: Piotr Drąg
  * Updated pt_BR: Enrico Nicoletto
  * Updated sl: Matej Urbančič


NEW in 0.15.2 - 2013-02-11
==========================

The changes are:

  * Build: Required SQLite 3.7.9 (was 3.7.0) for FTS4 support
  * New Feature: Full Text Search (FTS) v4 support:
                 New SPARQL function fts:snippet(?u)
                 - Snippets provide context around a matched query.
                 New SPARQL function fts:offsets(?u):
                 - Offsets provide an index where a matched query is found in context
                   See tracker-needle for basic use.
  * Fixes: GB#691847, tracker-extract: Port to new gupnp-dlna API
  * Fixes: GB#692409, playlist extractor should index playlist title if available
  * Fixes: GB#692603, Search can not find pdf files using author names
  * Ontology: Deprecated tracker:fulltextNoLimit and removed use in all other ontologies
  * libtracker-fts: Removed tracker_fts_config_min_word_length() and config option
  * libtracker-miner: Fixed GNOME password provider, libsecret expects "user" not "username"
  * tracker-info: Fixed memory leak where urn wasn't freed.
  * tracker-info: Fixed memory leak when using prefixes for shorthand properties

Translations:

  * None

Notes:

  The database version has been incremented, this will force a
  reindex for any existing Tracker installation.

  Due to the deprecation of tracker:fulltextNoLimit and removal of
  libtracker-fts's config option to set the minimum word length, we
  now index ALL small words and rely on stop words to filter
  redundant data. We still have the maximum word length checks in
  place.


NEW in 0.15.1 - 2013-01-24
==========================

The changes are:

  * build: Fixes build error when supplying nautilis extensions directory
  * Fixes: GB#691807, Tracker should build against libgee 0.8
  * Fixes: GB#691804, tracker-cue-sheet not ported to Gstreamer 1.0
  * Fixes: GB#679870, libsecret migrations
  * Fixes: GB#690170, Add nfo:MediaList type to .m3u playlist files
  * Fixes: GB#692210, miner-manager: plug some memleaks
  * libtracker-common: Add replacement function for flock() on Solaris
  * libtracker-miner: Fix libsecret password provider get call using non-NULL GError
  * tracker-miner-fs, store: Add MeeGo keys to desktop files

Translations:

  * Updated pt_BR: Henrique P. Machado


NEW in 0.15.0 - 2012-11-23
==========================

The changes are:

  * Require: GStreamer >= 1.0 (was 0.10) to follow upstream releases
  * Require: libgrss >= 0.5 (was 0.3) to follow upstream releases
  * Removed: Dia dependency for producing documentation.
  * New Feature: Support GKqueueDirectoryMonitor API on BSD systems

Translations:

  Many, thank you to everyone involved.



NEW in 0.14.0 - 2012-03-08
==========================

The changes are:

  * Require: libxml2 for --enable-maemo builds
  * Require: Vala >= 0.13.4 (for introspection fixes)
  * Require: libosinfo >= 0.0.2 for --enable-libosinfo (file ISO extractor)
  * Removed: ioprio_get() check in configure, now most kernels have the syscall
  * Removed: dbus-glib requirement (was only used in 1 or 2 cases)
  * Removed: GLib unicode parser, it's was sub-optimal and unused
  * New Feature: CUE sheet parsing is now supported
  * New Feature: ISO file extractor
  * libtracker-common: New env var TRACKER_USE_CONFIG_FILES to output ini files instead of use GSettings is available
  * libtracker-miner: Updated TrackerMinerFS (breaks API/ABI)
  * libtracker-miner: Introducing TrackerIndexingTree (to use with TrackerMinerFS)
  * libtracker-miner: Albumart renamed to Media art and refactored
  * tracker-miner-fs: Refactored, mtime checks are +50% faster and indexing slightly improved
  * tracker-needle: Now results are sorted nicely

GNOME bugs fixed (13):

  https://bugzilla.gnome.org/buglist.cgi?quicksearch=670679,670778,537899,671314,670251,670294,669646,669546,666372,667786,667787,666654,666410

Nokia bugs fixed (4):

  https://projects.maemo.org/bugzilla/buglist.cgi?quicksearch=296488,289635,296009,290406

Translations:

  Many, thank you to everyone involved.


NEW in 0.13.1 - 2012-02-15
==========================

The changes are:

  * Require: libosinfo >= 0.0.2 for --enable-libosinfo (file ISO extractor)
  * New Feature: ISO file extractor
  * Fixes: GB#669546, tracker-miner-fs: Segmentation fault
  * Fixes: GB#666654, unbreak compilation if the O_NOATIME flag for open(2) is not available
  * Fixes: GB#666410, missing gmodule detection for LIBTRACKER_EXTRACT
  * Fixes: GB#666678, g_str_has_prefix() called on a NULL string with tracker_config_file_new()
  * Fixes: GB#666372, More information about bootable ISOs
  * Fixes: GB#667786, Fix compile warning in tracker-file-notifier-test
  * Fixes: GB#667787, Fix tracker-file-notifier-test
  * Fixes: NB#296488, Fix crash when SIGTERM happens before mainloop is available
  * Fixes: NB#289635, Test for reply being NULL and don't unref in that case
  * Fixes: NB#296009, Add debugging aid for when opening MSOffice files fail
  * Fixes: NB#290406, Timeout PDF content extraction after 10s
  * build: Use g_thread_try_new() if GLib >= 2.31
  * build: Use g_(mutex|cond)_(init|clear) if GLib >= 2.31
  * build: Use GPrivate if GLib >= 2.31 (GStaticPrivate now deprecated)
  * build: Use GMutex if GLib >= 2.31 (GStaticMutex now deprecated)
  * build: Use g_atomic_int_add() if GLib >= 2.30
  * build: Use GRecMutex if GLib >= 2.31 (GStaticRecMutex now deprecated)
  * build: Use g_format_size() if GLib >= 2.30
  * build: Remove remaining g_thread_init() calls
  * build: Set _REQUIRED versions for giff, jpeg and tiff
  * build: Depend on gmodule for libtracker-extract since glib 2.31.4 doesn't depend on it
  * libtracker-common: Add new function tracker_file_open_fd()
  * libtracker-data: Fix dead code with disabled journal
  * libtracker-miner: Don't send sparql to tracker-store for missing/cancelled files
  * libtracker-miner: Ensure we deal with the canonical copy on ::directory-removed
  * libtracker-miner: Improve logging
  * libtracker-extract: Do not use tracker_coalesce_strip on constant strings in unit tests
  * libtracker-extract: Don't build EXIF tests if we don't have support enabled
  * tracker-store: Print warning if unable to initialize statistics
  * tracker-miner-fs: Fix crash when SIGTERM happens before mainloop is available
  * tracker-miner-fs: Don't index removable media on initial index if disabled
  * tracker-extract-msoffice: Add debugging aid for when opening file fails
  * tracker-needle: Fix build with GLib 2.31
  * ttl2sgml: Do not call fclose if file is NULL

Translations:

  * Updated fr: Alexandre Franke
  * Updated ja: Jiro Matsuzawa
  * Updated nb: Kjartan Maraas


NEW in 0.13.0 - 2011-12-15
==========================

The changes are:

  * Require: libxml2 for --enable-maemo builds
  * Require: Vala >= 0.13.4 (for introspection fixes)
  * Removed: ioprio_get() check in configure, now most kernels have the syscall
  * Removed: dbus-glib requirement (was only used in 1 or 2 cases)
  * Removed: GLib unicode parser, it's was sub-optimal and unused
  * New Feature: CUE sheet parsing is now supported
  * libtracker-common: New env var TRACKER_USE_CONFIG_FILES to output ini files instead of use GSettings is available
  * libtracker-miner: Updated TrackerMinerFS (breaks API/ABI)
  * libtracker-miner: Introducing TrackerIndexingTree (to use with TrackerMinerFS)
  * libtracker-miner: Albumart renamed to Media art and refactored
  * tracker-miner-fs: Refactored, mtime checks are +50% faster and indexing slightly improved

  Many other fixes

Translations:

  Many, thank you to everyone involved.


NEW in 0.12.0 - 2011-09-09
==========================

The changes are:

  * Require: gstreamer-pbutils when using discoverer backend
  * Require: GSettings (not optional)
  * Require: GObject Introspection 0.9.5 (not optional from git, optional from tarball)
  * Require: GLib 2.28 was 2.26 (for modern APIs like g_slist_free_full())
  * Require: GTK+ 3.0 was 2.18.0 (for UI components only)
  * Require: Poppler-GLib 0.16.0 was Poppler 0.12.2 (old API was no longer supported)
  * Require: Evolution, EDS, Camel 2.32.0 was 2.91.90 (API changes)
  * Require: rest 0.7 was 0.6 (for Flickr distro library updates)
  * Require: GStreamer 0.10.31 was 0.10.12 (for GstDateTime)
  * Removed: Helix extractor for Real Media (it's covered by the GStreamer extractor)
  * Removed: libtracker-client (was deprecated in 0.10)
  * New Feature: Firefox >= 4.0 support
  * New Feature: Thunderbird >= 5.0 support
  * New Feature: Add --disable-journal configure option, previously it didn't exist
  * New Feature: Support MeeGoTouch (for application extraction)
  * New Feature: SPARQL: Add support for HAVING
  * New Feature: SPARQL: Add support for COALESCE
  * New Feature: SPARQL: Add support for INSERT OR REPLACE
  * New Feature: Region of interest support in image formats (for example tagging people in images)
  * libtracker-common: Do not delete old log files when starting components
  * libtracker-data: Switch to manual SQLite WAL checkpointing in a separate thread
  * libtracker-sparql: Remove deprecated drop_graph API
  * tracker-miner-fs: Improve query performance for updating tracker:available on mounting
  * tracker-miner-fs: Handle desktop files of Type=Link
  * tracker-miner-fs: Support local (in $HOME) XDG directories for desktop files
  * tracker-extract: Completely relicensed all modules and the extractor to LGPL from GPL
  * tracker-extract: Use two threads now for extraction, a controller and worker thread
  * tracker-extract: Refactored the module management so modules can state mimes supported flexibly
  * tracker-extract: Support extra string for WHERE and GRAPH patterns when calling modules
  * tracker-extract: Added EPub extractor module
  * tracker-writeback: Refactor writeback so miner-fs performs more of the operation itself
  * tracker-writeback: Deprecate IgnoreNextUpdate API
  * tracker-writeback: Support longitude, latitude and altitude information to XMP files
  * tracker-control: Report remaining estimated time for each miner's progress
  * tracker-control: Added --set-log-verbsosity to change component log verbosity
  * tracker-control: Added --pause-for-process for potentially crash worthy processes
  * tracker-search: Support bookmarks with -b or --bookmarks

GNOME bugs fixed (44):

https://bugzilla.gnome.org/buglist.cgi?quicksearch=658588,658232,657354,655898,657161,654824,656838,655987,655789,655701,654653,654441,654407,652433,633105,397205,520670,642288,633108,632876,646834,647548,642883,644997,646365,646374,645380,611471,645934,645963,645675,642012,645000,645052,645375,644998,642868,622288,642774,643739,636375,643140,642581,643090

Nokia bugs fixed (81):

https://projects.maemo.org/bugzilla/buglist.cgi?quicksearch=281201,277052,281183,281540,271437,272441,279789,256958,276858,276830,242979,278806,277799,278171,246131,247468,259325,271389,254896,274498,274181,271747,273120,269986,270774,259942,269766,269931,268105,269359,266579,251032,263203,261635,258488,258459,254855,259000,252898,244556,254154,248873,253807,249338,250959,250455,249787,249028,244787,240681,238770,245998,244267,241302,245589,245373,213419,244536,242253,229461,237582,240272,241659,241833,235445,241206,240168,240582,237176,238447,238547,237150,236387,233828,232574,235118,229713,232981,231631,230255,228760

Translations:

  Many, thank you to everyone involved.

Notes:

  The database version has been incremented, this will force a
  reindex for any existing Tracker installation.

  Existing configurations should have migration paths to new GSettings
  based format.

  Of all the requirement changes listed above, the only GSettings and
  GObject Introspection are absolutely required to build Tracker, the
  others are optional depending on components being built. These are
  also only when building from git, not a distributed tarball (which
  should have the m4 macros packaged).


NEW in 0.11.2 - 2011-08-16
==========================

The changes are:

  * Build: Bump tracker_api_version to 0.12 but keep major/miner as 0.11 (for so names)
  * Build: New --with-gstreamer-backend=[tagreadbin,decodebin2,discoverer] switches
  * Require: gstreamer-pbutils when using discoverer backend
  * Removed: Helix extractor for Real Media (it's covered by the GStreamer extractor)
  * Fixes: GB#655987, VAPI file lacks annotation for MinerManager
  * Fixes: GB#655789, Some TrackerMinerManager methods lack introspection annotations
  * Fixes: GB#655701, Fix build system for firefox/thunderbird plugins to work for packaging
  * Fixes: GB#654441, Rename --enable-video-extractor to --enable-generic-media-extractor
  * Fixes: NB#247468, Ontology missing for message delivery status when sending while remote contact is offline
  * functional-tests: Disable optical and removable indexing for tests
  * libtracker-extract: Don't install examples/ rules mock up
  * libtracker-extract: Added some debugging to know what rules files are loaded
  * tracker-extract: Make sure we check filtered modules for dbus calls too
  * tracker-extract: Improve checks for mimetype guessing (didn't check empty strings)
  * tracker-extract: Improve debugging for filtered modules and mime guessing
  * tracker-extract: Fix --force-module for commandline extraction
  * tracker-extract: Make JPEG module handle DLNA profiles
  * tracker-extract: Make PNG module handle DLNA profiles
  * tracker-extract: Make MP3 module handle DLNA profiles
  * tracker-extract: Let gstreamer extractor handle 'dlna/*' mimetypes
  * tracker-extract-gstreamer: Compiler warning fixes and code refactoring
  * tracker-extract-gstreamer: Assume UTC for dates
  * tracker-miner-fs: Disable writeback by default (was enabled accidentally)
  * tracker-miner-flickr: Remove deprecated Encoding key in desktop file
  * tracker-needle: Avoid g_markup_escape_text() critical with NULL
  * tracker-needle: Show email subject as primary information not sender
  * tracker-needle: Fix build with Vala master
  * thunderbird: Add desktop file so needle opens email on activation
  * thunderbird: Use asynchronous connection opening to avoid blocking the UI
  * thunderbird: Don't break on consecutive make installs, use -f with ln
  * firefox: Use asynchronous connection opening to avoid blocking the UI
  * firefox: Raise compatibility level to 5.0.*
  * firefox: Don't break on consecutive make installs, use -f with ln

Translations:

  * Updated cs: Marek Černocký
  * Updated de: Christian Kirbach
  * Updated es: Daniel Mustieles
  * Updated ru: Yuri Myasoedov
  * Updated sl: Andrej Žnidaršič
  * Updated sv: Daniel Nylander

Notes:

  None


NEW in 0.11.1 - 2011-08-01
==========================

The changes are:

  * New Feature: Firefox >= 4.0 support
  * New Feature: Thunderbird >= 5.0 support
  * New Feature: Add --disable-journal configure option, previously it didn't exist
  * Fixes: GB#654653, miner: don't use G_CONST_RETURN
  * Fixes: GB#654407, sqlite: implement get_property() for n-columns
  * Fixes: GB#652433, Progress signal gives faulty value just before finishing
  * Fixes: NB#271389, Extraction of album art fails for a particular .mp3 file
  * Fixes: NB#254896, tracker-miner-fs should resume pause if pausing clients crash
  * Fixes: NB#274498, Adding tag to song causes update quad flood
  * Fixes: NB#274181, Attached file's album art is not extracted right
  * Fixes: NB#271747, [TASK] Support xsd:date
  * Fixes: NB#273120, After disconnecting from USB mass storage mode apps takes a long time to display content
  * Fixes: NB#269986, Poor performnace with different music files
  * Fixes: NB#270774, [TASK] Improve UpdateArray performance
  * Fixes: NB#259942, Corrupted symbols for Cyrillic mp3 tags
  * Fixes: NB#269766, Make TrackerProcessingTask refcounted to avoid double free
  * Fixes: NB#269931, Some particular JPG files are never indexed by Tracker
  * Fixes: NB#268105, [TASK] Disabled full journaling to improve user data privacy
  * Fixes: NB#269359, tracker-control got crashed at tracker_control_status_run   when the file system is full
  * Fixes: NB#266579, tracker_sparql_connection_get starts emitting GLIB CRITICALS when called again after failure
  * Fixes: NB#251032, Corrupted thumbnails are displayed for few minutes when multiple images are deleted
  * Fixes: NB#263203, tracker-store fails to start when out of disk space
  * Fixes: NB#261635, Improve pinyin sorting
  * Ontology: Set tracker:writeback true for nfo:Heading
  * SPARQL: Add support for HAVING
  * SPARQL: Add support for COALESCE
  * libtracker-data: Fix leak in db_get_locale
  * libtracker-data: Do not print critical if db-locale.txt does not exist
  * libtracker-data: Switch to manual SQLite WAL checkpointing in a separate thread
  * libtracker-data: Make sure we don't leak sql_dir on multiple _init() calls
  * libtracker-miner: translate monitor events CREATE(a)+MOVE(a->b)=UPDATE(b)
  * libtracker-miner: Make sure status is set to 'Idle' on object construction
  * libtracker-miner: Fix inconsistent status with progress signals
  * libtracker-miner: Fix -0.00 progress being reported
  * libtracker-miner: Make sure "Processing..." is 2% or higher in progress
  * libtracker-extract: Use nfo:heading instead of nmm:direction
  * libtracker-extract: Support extraction of EXIF/XMP GPS information for images
  * libtracker-extract: Ignore keywords containing invalid UTF-8
  * libtracker-extract: Do not use G_CONST_RETURN
  * tracker-store: Check XDG directories after initializing log to reduce start up stdout spam
  * tracker-store: Improve UpdateArray performance
  * tracker-extract: MP3: Use guessed ID3v1 8-bit encoding also for ID3v2
  * tracker-extract: Playlist: Limit playlists to 1000 entries
  * tracker-extract: handle RealMedia files with the GStreamer extractor
  * tracker-writeback: Refactor writeback so miner-fs performs more of the operation itself
  * tracker-writeback: Deprecate IgnoreNextUpdate API
  * tracker-writeback: Support longitude, latitude and altitude information to XMP files
  * tracker-writeback: Always clear existing XMP fields before updating metadata
  * tracker-miner-fs: Check XDG directories after initializing log to reduce start up stdout spam
  * tracker-control: Added --pause-for-process for potentially crash worthy processes
  * tracker-control: Fix crash when unable to get miner pause details
  * tracker-search: Support bookmarks with -b or --bookmarks
  * tracker-preferences: Fix apply button

Translations:

  * Updated cs: Marek Černocký
  * Updated es: Daniel Mustieles
  * Updated nb: Kjartan Maraas
  * Updated pt_BR: Gabriel Speckhahn
  * Updated sl: Andrej Žnidaršič
  * Updated sv: Daniel Nylander
  * Updated zh_CN: Yinghua Wang

Notes:

  None


NEW in 0.11.0 - 2011-06-07
==========================

The changes are:

  * Require: GSettings (not optional)
  * Require: GObject Introspection 0.9.5 (not optional)
  * Require: GTK+ 3.0 was 2.18.0 (for UI components only)
  * Require: Poppler-GLib 0.16.0 was Poppler 0.12.2 (old API was no longer supported)
  * Require: Evolution, EDS, Camel 2.32.0 was 2.91.90 (API changes)
  * Require: rest 0.7 was 0.6 (for Flickr distro library updates)
  * Require: GStreamer 0.10.31 was 0.10.12 (for GstDateTime)
  * Removed: libtracker-client (was deprecated in 0.8)
  * Removed: GConf (replaced by GSettings)
  * Support: MeeGoTouch (for application extraction)
  * libtracker-sparql: Remove deprecated drop_graph API
  * tracker-extract: Completely relicensed all modules and the extractor to LGPL from GPL
  * tracker-extract: Use two threads now for extraction, a controller and worker thread
  * tracker-extract: Refactored the module management so modules can state mimes supported flexibly
  * tracker-extract: Support extra string for WHERE patterns when calling modules
  * tracker-extract: Much improvement to the logging
  * tracker-extract: Added EPub extractor module
  * tracker-control: Report remaining estimated time for each miner's progress

Translations:

  * Updated ca: Gil Forcada
  * Updated cs: Marek Černocký
  * Updated da: Joe Hansen
  * Updated de: Mario Blättermann, Markus Schley
  * Updated el: Georgios Stefanakis, Kostas Papadimas
  * Updated eo: Kristjan SCHMIDT
  * Updated es: Daniel Mustieles
  * Updated fr: Claude Paroz
  * Updated hu: Gabor Kelemen
  * Updated pl: Piotr Drąg
  * Updated pt_BR: Gabriel F. Vilar
  * Updated sl: Andrej Žnidaršič
  * Updated sv: Daniel Nylander
  * Updated zh_CN: Yinghua Wang

Notes:

  The database version has been incremented, this will force a
  reindex for any existing Tracker installation.

  Existing configurations should have migration paths to new GSettings
  based format.

  Of all the requirement changes listed above, the only GSettings and
  GObject Introspection are absolutely required to build Tracker, the
  others are optional depending on components being built. These are
  also only when building from git, not a distributed tarball (which
  should have the m4 macros packaged).


NEW in 0.10.0 - 2011-02-17
==========================

The changes are:

  * Require: libtool >= 2.2 (previously no required version)
  * Require: autoconf >= 2.64 was 2.59
  * Require: automake 1.11 and use Vala features
  * Require: Vala >= 0.11.4 for GDBus / sender support
  * Require: GLib 2.26
  * Require: D-Bus 1.3.1 or higher
  * Require: SQLite 3.7 (for Write Ahead Logging)
  * Require: libicu or libunistring (GLib unicode support has performance issues)
  * Require: poppler 0.12.2 instead of poppler-glib 0.4.5
  * New Feature: Added GDBus support
  * New Feature: Added direct access library libtracker-sparql for fast read-only queries
  * New Feature: Added LRU (Least Recently Used) for database statement cache (huge memory footprint difference)
  * New Feature: Added a GUPnP DLNA extractor
  * New Feature: Added domain specific indexes, adding a column to DB tables to avoid full table scans/joins with larger tables for performance.
  * New Feature: Added metadata guarantee support when key data is missing (e.g. nie:title generation from filename)
  * New Feature: Added tracker-sql (in utils/) to run SQL on the databases directly (for developers)
  * New Feature: Added unicode compliance for FTS (Full Text Search)
  * New Feature: Don't do mtime checks on start up (dramatically reduces start time for miner-fs)
  * New Feature: Replaced tracker-search-tool with tracker-needle
  * New Feature: New API methods in libtracker-sparql which allow multiple inserts per DBus call (and yet get multiple errors back).
  * New Feature: New journal format to separate ontology from data for improved data integrity
  * New Feature: New journal compression and rotation using 50Mb chunk sizes
  * New Feature: Task pool in libtracker-miner for store updates/metadata extractions, 10-20% speed increase indexing thanks to the work from Zhenqiang Chen
  * New Feature: Monitor event black listing and events are condensed (less event spam up the stack)
  * New Feature: Return types in TrackerSparqlCursor API (previously just strings)
  * New Feature: Collation support
  * New Feature: Refactored and much more efficient signal notification API
  * New Feature: Use libtracker-sparql: Client library for direct/dbus access
  * New Feature: SPARQL, Add support for EXISTS and NOT EXISTS
  * New Feature: SPARQL, Add support for IN
  * New Feature: SPARQL, Add support for INSERT SILENT and DELETE SILENT
  * Updated totem plugin to use libtracker-sparql
  * Updated evolution plugin to recent versions of Evolution/EDS
  * Updated GTK+ search engine to use libtracker-sparql
  * Updated tracker-search bar to be GTK+ 3 / GNOME 3 compliant
  * Removed tracker-status-icon
  * Merged tracker-status into tracker-control
  * libtracker-client is now marked as deprecated

GNOME bugs fixed (102):

https://bugzilla.gnome.org/buglist.cgi?quicksearch=640845,639547,641906,640623,636074,631955,623666,639542,622918,639918,639556,639543,639552,600081,638159,636936,624041,636604,635756,636383,635794,635757,634424,630244,635652,634669,633118,633093,631391,575993,628872,628859,628090,628536,627893,613252,627281,623005,623207,624731,616199,624346,624071,623836,622702,623203,616809,511474,621070,621987,620354,620362,622351,621108,621110,621015,619244,621183,621112,621001,620353,619498,619929,619535,619826,619801,619615,619630,619271,560220,616540,526346,617812,619487,503366,619061,616845,618822,618707,618821,618544,618011,614728,616808,617515,617082,579051,615930,616493,615836,616165,616403,616329,615948,616158,615868,615035,615178,615563,615184,615313,615182

Nokia bugs fixed (154):

https://projects.maemo.org/bugzilla/buglist.cgi?quicksearch=227977,208621,223536,225910,221340,180770,217610,224993,222182,224760,218525,224763,197067,223938,185309,223622,223282,223133,223148,223161,222645,222494,222664,222578,221700,220373,218092,218332,217636,218125,219309,216582,218560,211147,218123,216329,217575,198003,216350,212736,214163,214478,214413,213419,211928,212116,210217,211926,198155,207918,208420,203538,204706,202291,206752,203540,203593,195787,203632,202275,199968,202627,202581,201122,200310,199131,194908,194548,186096,197546,197097,195394,196143,194677,195547,192790,193044,194746,193061,186055,190480,192326,188031,184220,187558,189896,188494,184505,187149,174794,185950,185070,182948,184882,184823,177663,181421,181112,183307,172390,179570,180803,182788,181538,179894,180236,180679,179434,179559,179798,176090,178599,178825,178808,174590,177447,176932,175723,170915,175957,175712,174591,174333,174162,173673,172818,174248,173589,173164,172414,161449,173206,172652,171657,167647,170142,168740,164822,169460,163155,169254,169105,169119,168742,167616,166767,166717,166684,166080,164958,161047,163544,163715,163480

Translations:

  Many, thank you to everyone involved.

Notes:

  The database version has been incremented, this will force a
  reindex for any existing Tracker installation.


NEW in 0.9.38 - 2011-02-10
==========================

The changes are:

  * Fixes: NB#208621, SEV-1: 10776 Music GSM-BTR-1-8108 Music Player: Duplicates in the store when synchronizing with Ovi Suite/WMP
  * Fixes: NB#223536, tracker-extract segfaults when songs without album or artist metadata is copied to device
  * Fixes: NB#225910, tracker-miner is crashing frequently
  * Fixes: NB#221340, Modified album art is not updated after synchronizing with WMP.
  * Fixes: NB#180770, Unusable emergency contact is found in Search application
  * Fixes: NB#217610, Generated album art shown for some clips with albumart metadata
  * Fixes: NB#224993, after ~440 smses tracker does not respond any longer
  * Fixes: NB#222182, tracker_sparql_connection_get_direct should fail if direct access is not possible
  * Fixes: NB#224760, "too many sql variables" when "filter ?sth in (long list)"
  * Fixes: GB#640623, Double-click on application result should start the application
  * ontology: Make nie:url an inverse functional property (essentially making it unique)
  * functional-tests: Kill/stop the store in a realistic way for backup/restore
  * documentation: Various fixes for libtracker-{client|miner|extract|sparql}
  * libtracker-data: Added unit tests for IN with > 20 elements 
  * libtracker-data: Improve error handling on initialization (especially if the WAL PRAGMA fails)
  * libtracker-data: Fixed a race condition on shutdown causing criticals with  g_object_unref()
  * tracker-store: Increase maximum D-Bus result size (1 Mb to 10 Mb)
  * tracker-store: Don't send unmapped errors in D-Bus replies
  * nautilus: Plugin has been updated to use libtracker-sparql (was libtracker-client)

Translations:

  * Updated ar: Khaled Hosny
  * Updated cs: Marek Černocký
  * Updated es: Daniel Mustieles
  * Updated sv: Daniel Nylander

Notes:

  The database version has been incremented, this will force a
  reindex for any existing Tracker installation.


NEW in 0.9.37 - 2011-02-03
==========================

The changes are:

  * New feature: Don't do mtime checks on start up (dramatically reduces start time for miner-fs)
  * Fixes: NB#198003, Don't crash in 'tracker_miner_init', support GInitable and handle errors gracefully 
  * Fixes: NB#224342, tracker-control should have options to pause and resume the miners
  * Fixes: NB#218525, tracker-miner is too heavy to start
  * Fixes: NB#224763, GLIB CRITICAL ** GLib-GIO - g_dbus_proxy_call: assertion `parameters == NULL || g_variant_is_of_type (parameters, G_VARIANT_TYPE_TUPLE)' failed
  * Fixes: NB#197067, tracker-miner-fs crawls open files
  * Fixes: NB#223938, Don't crash in tracker-extract when using QImage for SVG files
  * Fixes: NB#185309, Extract track numbers from MP3 ID3v2.2 frames
  * Fixes: NB#223622, Tracker should update mtime of the "db-version.txt" stamp file after a backup restoration
  * Fixes: NB#223282, --enable-guarantee-metadata and PNG: no nie:contentCreated
  * Fixes: NB#223133, GLIB CRITICAL ** Tracker - file tracker-bus.c: line 2700: uncaught error: GDBus.Error:org.gtk.GDBus.UnmappedGError.Quark._tracker_5fdate_5ferror_2dquark.Code1
  * Fixes: NB#223148, Don't crash in tracker-extract when passing NULL artist/album to g_variant_new()
  * Fixes: NB#223161, tracker-miner-fs crashes trying to g_object_unref a GError
  * Fixes: NB#222645, Inserting a resource using an non-existing class, doesn't rollback completely
  * Fixes: NB#222494, Tracker writeback uses CPU while recording video with camera
  * Fixes: GB#636074, libunistring and glib collations break fn:starts-with
  * Fixes: GB#631955, drop status icon use
  * Fixes: GB#623666, Change the 3 days time for removable media information deletion to a configuration value
  * Fixes: GB#600081, Get rid of Bonobo
  * functional-tests: Added tests for interaction with applications creating content
  * functional-tests: Added parameter to start the tracker processes manually
  * libtrakcer-data: We now restore tracker:modified values after journal replay and backup restore
  * libtracker-data: Don't journal non-embedded data (e.g. nie:plainTextContent)

Translations:

  * Updated cs: Marek Černocký
  * Updated es: Daniel Mustieles
  * Updated et: Ivar Smolin
  * Updated fr: Bruno Brouard
  * Updated sl: Andrej Žnidaršič

Notes:

  The command 'tracker-status' no longer exists, its command line
  features have been merged into 'tracker-control'.


NEW in 0.9.36 - 2011-01-27
==========================

The changes are:

  * Fixes NB#221700: miner-fs: Handle errors when requesting bus names
  * Fixes NB#222578: tracker-writeback: Add member= to the signal's subscription
  * Fixes NB#222645: libtracker-data: Fix resource without URI on insert after rollback
  * Fixes NB#222664: miner-fs: Do not access MemoryOutputStream data before finishing splice
  * Ontology: Added missing documentation for rdf, mtp and slo
  * Ontology: Remove long-deprecated classes and properties
  * SPARQL: Ignore CONSTRUCT/INSERT/DELETE triples with unbound variables
  * SPARQL: Fix SQL generated for predicate variables and deleted subjects
  * libtracker-data: Fix transient properties
  * functional-tests: Ensure nfo:belongsToContainer is changed on file moves

Translations:

  * Updated ar: Khaled Hosny
  * Updated cz: Marek Cernocky

Notes:

  None


NEW in 0.9.35 - 2011-01-20
==========================

The changes are:

  * Require: Vala >= 0.11.4 for GDBus sender support and bug fixes
  * build: All D-Bus use has been ported to GDBus (except libtracker-client and tracker-writeback)
  * Fixes: NB#220373, nie:contentCreated is set for music files that do not have the year set in metadatas
  * Fixes: NB#218092, [REG][Performance] Conversation view opening time is above performance target value
  * Fixes: NB#218332, The song duration is always shown as 0.00 in all views, except the player view
  * Fixes: NB#217636, Not able to delete contact using DELETE {<contact:556> ?p ?v} WHERE {<contact:556> ?p ?v}
  * Fixes: NB#218125, Inconsistency between nco:photo and nco:video
  * Fixes: NB#219309, tracker-store reads /proc/mounts 6 times & does 6*13 stat() calls when swiping to next image in gallery
  * Fixes: NB#216582, Tracker-store does not handle slo:location tags inserted into virtual file
  * Fixes: GB#639542, tracker-needle doesn't support any argument
  * Fixes: GB#622918, Migrate from dbus-glib to glib's GDBus
  * Fixes: GB#639918, Since merge of gdbus branch, build process is broken in tracker-evolution-plugin.
  * Fixes: GB#639556, Tracker find only folders
  * Fixes: GB#639543, Ctrl+W should close the window
  * Fixes: GB#639552, Give needle window a name that means something
  * SPARQL: Add support for IF function
  * Ontology: Added nmo:lastSuccessfulMessageDate property
  * Ontology: Added domain index on nmo:sentDate to nmo:Call
  * Ontology: Fixed NCO cyclic dependency with NFO
  * Ontology: Fixed range of nco:photo and nco:video, fixed maxCardinality of nco:video
  * Ontology: Added county and district to postal address
  * Ontology: Draft the NCO documentation page
  * libtracker-data: Remove unused TrackerDBResultSet
  * libtracker-common: Use statfs instead of statvfs on Linux
  * libtracker-miner: Log when an event is ignored because the file is locked
  * tracker-writeback: unlock file after 1s of update, not 3s
  * tracker-writeback: xmp: Don't use XMP_CLOSE_SAFEUPDATE
  * tracker-extract: gupnp-dlna: Update to gupnp-dlna 0.5 API
  * tracker-extract: mp3: Fixed bitrate table and sample rate table usage
  * tracker-extract: mp3: Cope with sample_rate = 0
  * tracker-needle: Support command line arguments -V and search criteria
  * tracker-needle: List folders in first/initial view
  * tracker-needle: Add Ctrl+W to close window
  * tracker-needle: Use a meaningful window title "Desktop Search"

Translations:

  * Updated id: Andika Triwidada

Notes:

  None


NEW in 0.9.34 - 2011-01-13
==========================

The changes are:

  * Fixes NB#198003: libtracker-miner: Improve SPARQL connection error handling
  * Fixes NB#211147: libtracker-data: Update domain indexes on type statement insert
  * Fixes NB#216329: tracker-store: Reinitialize signals after failed backup or restore
  * Fixes NB#216350: libtracker-data: Fix double free in ontology cache code
  * Fixes NB#217575: libtracker-data: Update rdf:type table when deleting type statements
  * Fixes NB#218123: SLO: Fixed maxCardinality of some properties
  * Fixes NB#218560: Fix assertion failure when ontology cache fails to load
  * Ontology: Make SLO an ontology that is installed by default
  * Ontology: Fixed maxCardinality and deprecated annotations in MLO
  * libtracker-data: Fix double unref on shutdown
  * libtracker-data: Fix memory leak in gvdb writing
  * libtracker-data: Update statistics when bypassing buffer for deletes
  * libtracker-data: Fix tracker_ontologies_get_classes
  * libtracker-sparql: Shutdown backends when finalizing connection
  * libtracker-common: Improve tracker_dbus_request_* API
  * tracker-extract: Relicense from GPL to LGPL
  * tracker-extract: TIFF: SLO ontology porting
  * tracker-extract: PNG: SLO ontology porting
  * tracker-extract: PDF: SLO ontology porting
  * tracker-extract: JPEG SLO ontology porting
  * tracker-extract: GIF: SLO ontology porting
  * tracker-writeback: SLO ontology porting
  * tracker-needle: Use nco:publisher if nco:creator is not available for documents

Translations:
  * Updated cz: Marek Cernocky
  * Updated de: Mario Blättermann
  * Updated pt_BR: Mateus Zenaide
  * Updated se: Daniel Nylander
  * Updated sl: Andrej Žnidaršič, Matej Urbančič

Notes:

  None


NEW in 0.9.33 - 2010-12-29
==========================

The changes are:

  * Fixes NB#211928: SPARQL: Do not use nested subquery for EXISTS / NOT EXISTS
  * Fixes NB#212736: SPARQL: Improve predicate variable support
  * Fixes NB#213419: SPARQL: Implicitly convert fn:concat arguments to string
  * Fixes NB#214478: libtracker-data: Fix memory leak in cursor_next_async
  * Fixes GB#638159: libtracker-data: Fix gvdb lookup for not existing ontology resources
  * Ontology: Remove tracker:isAnnotation property
  * SPARQL: Support graph retrieval using property functions
  * SPARQL: Accept semicolon as separator between update operations
  * SPARQL: Add support for INSERT/DELETE DATA
  * SPARQL: Fix WITH support
  * SPARQL: Drop deprecated support for DROP GRAPH
  * libtracker-data: Add ontology cache
  * libtracker-data: Bypass update buffer for rdf:type deletes if possible
  * tracker-store, libtracker-data: Enhance signal behaviour for batch updates
  * libtracker-common: Print PID of process in dbus requests
  * libtracker-miner: Set proper nfo:belongsToContainer on file move operations
  * libtracker-miner: Skip IgnoreNextUpdate requests on CREATED events
  * libtracker-miner: When deleting resources, first mark unavailable and then fully remove them
  * libtracker-miner: Make sure original monitors are removed during a move operation
  * libtracker-miner: On directory remove, first remove monitors, then notify event
  * libtracker-miner: When a moved dir event detected, cancel all monitors in the dir
  * libtracker-miner: Don't add monitors when renaming a directory to a hidden name
  * tracker-extract-gstreamer: Fix undefined references
  * tracker-extract-txt: Support text files encoded in UTF-16 if BOM available
  * tracker-writeback: XMP: Take into account mlo:asPostalAddress
  * tracker-writeback: Fix a memoryleak on unwanted_results and cleanup
  * tracker-writeback, libtracker-miner: Don't writeback non-strict mined values
  * tracker-explorer: Port to GDBus

Translations:
  * Updated cs: Marek Cernocky
  * Updated de: Mario Blättermann
  * Updated es: Daniel Mustieles
  * Updated et: Mattias Põldaru
  * Updated hu: Gabor Kelemen
  * Updated se: Daniel Nylander
  * Updated sl: Andrej Žnidaršič
  * Updated zh_CN: Aron Xu

Notes:

  None


NEW in 0.9.32 - 2010-12-16
==========================

The changes are:

  * New feature: Added tracker-needle to replace tracker-search-tool
  * New feature: Added --enable-guarantee-metadata to ensure nie:title and other metadata when missing
  * build: disable functional-tests by default
  * Fixes: NB#210217, Notification of nmo:Call class changes needed
  * Fixes: NB#211926, SVG images are represented as nmm:Photo not nfo:VectorImage
  * Fixes: NB#198155, libtracker-sparql doesn't return results if multiple async queries are run in parallel in direct back-end
  * Ontology: Add tracker:notify true for nmo:Call
  * libtracker-data: Sort function test results to avoid test failures
  * libtracker-sparql: Wait for tracker-store before loading plugins

Translations:
  * Updated cs: Petr Kovar

Notes:

  None


NEW in 0.9.31 - 2010-12-10
==========================

The changes are:

  * Require: autoconf >= 2.64 was 2.59
  * Require: libtool >= 2.2 (previously no required version)
  * build: Require Dia with --enable-gtk-doc
  * build: Require Python >= 2.6 with --enable-functional-test
  * build: Binaries built now use *stable* named versions only (i.e. 0.10 not 0.9)
  * Fixes: GB#636936, Require dia if we want to generate the documentation
  * Fixes: GB#624041, Port tracker to gtk+ 3.0
  * Fixes: GB#636604, tracker-0.9.30 linking errors.
  * Fixes: GB#635756, Include libtracker-data.vapi in tarball
  * Fixes: GB#636383, tracker-status-icon grabs control+alt+S keybinding unconditionally which causes issues for emacs
  * Fixes: GB#636520, Use upstream gettext instead the Glib one
  * Fixes: GB#636527, Use autoreconf instead gnome-autogen.sh
  * Fixes: NB#207918, ICO image formats are displayed in ...
  * Ontology: Added nmm:MusicAlbumDisk, nmm:musicAlbumDisk, nmm:albumDiskAlbum classes
  * Ontology: Changed nmm:performer rdfs:domain from nmm:MusicAlbum to nmm:MusicAlbumDisk
  * Ontology: Changed nmm:setNumber rdfs:domain from nmm:MusicAlbum to nmm:MusicAlbumDisk
  * Ontology: Changed nmm:internationalStandardRecordingCode rdfs:domain from nmm:MusicAlbum to nmm:MusicPiece
  * libtracker-sparql: Fixed documentation to include tracker_sparql_get_uuid_urn()
  * libtracker-miner: Fixed documentation missing but written
  * tracker-extract: Fixed GUPNP-DLNA extractor, was using "nfo.channels" not "nfo:channels"
  * functional-tests: Don't hardcode python version in scripts

Translations:

  None

Notes:

  None


NEW in 0.9.30 - 2010-12-02
==========================

The changes are:

  * Requires Vala 0.11.2 was 0.11.1
  * Fixes: NB#204706, Classes slo:Landmark, slo:LandmarkCategory and slo:GeoLocation should qualify for notifications
  * Fixes: NB#203538, libtracker-sparql: cannot run 2 async queries at the same time
  * Fixes: NB#208420, Critical in miner-fs when msyncd and Ovi Suite in operation
  * Fixes: GB#635757, missing test file in tarball
  * Fixes: GB#635794, build fails - missing .vapi files?
  * Ontology: Added domainIndex nie:contentCreated to nfo:Visual
  * Ontolgoy: Added tracker:notify true for slo:LandmarkCategory, slo:Landmark and slo:GeoLocation
  * SPARQL: Use WITH/INTO/FROM clause in WHERE pattern of updates
  * Support MeeGoTouch Locale, recalculating indexes on locale changes
  * data-generators: Diversify the content creation times
  * data-generators: Support nfo:Equipment correctly
  * data-generators: Small fixes to the music data
  * libtracker-data: Disable fn:starts-with test for !libicu cases
  * libtracker-data: Document the TRACKER_PRAGMAS_FILE environment variable
  * libtracker-direct: Make singleton connection thread-safe
  * libtracker-bus: Reduce D-Bus proxy initialization time
  * libtracker-sparql: Reduce D-Bus proxy initialization time
  * tracker-extract, albumart: Don't assume that g_file_get_parent() returns non-NULL
  * tracker-miner-applications: Re-extract desktop files when locale change detected
  * tracker-status: Added more granularity to the tracker-store status reporting for the journal replay

Translations:

  * fr: Bruno Brouard
  * pt_BR: Mateus Zenaide

Notes:

  None


NEW in 0.9.29 - 2010-11-24
==========================

The changes are:

  * Fixes: GB#630244, Use MaxBytes from tracker-extract.cfg for msoffice 2007 files
  * Fixes: GB#634424, MsOffice metadata extractor is not unicode aware
  * Fixes: GB#634669, Missing test file in tarball
  * Fixes: GB#635652, Correction in the --enable-tracker-search-tool configuration flag management
  * Fixes: NB#195787, libtracker-sparql, libtracker-client: Fix URI escaping
  * Fixes: NB#197067, tracker-miner-fs crawls open files
  * Fixes: NB#200980, Ontology: Add index on tracker:added
  * Fixes: NB#202291, tracker-extract: Fixed nested use of anonymous blank nodes
  * Fixes: NB#203540, Composer label not extracted in ID3v2.3 mp3 files
  * Fixes: NB#203593, All libtracker-miner section names in CamelCase in documentation
  * Fixes: NB#206752, SPARQL: Do not ignore GRAPH around rdf:type statements
  * Ontology: Add index on nco:nameFamily
  * SPARQL: Support \u escape sequence in string literals
  * SPARQL: Fix INSERT/DELETE and SELECT * with WHERE without variables
  * SPARQL: Use proper collation for string literals and fn:starts-with
  * libtracker-data: Make it easier to test the PRAGMAs for SQLite
  * libtracker-bus: Fix handling of priority in update_async
  * libtracker-sparql: Add function to generate a uuid urn to the client library
  * tracker-store: Fix crash on client exit and task without destroy notify
  * tracker-extract: Split msoffice and msoffice-xml extractors and refactor code
  * tracker-extract-mp3: Fix memory leak
  * tracker-miner-applications: Fix encoding issues and handling of nie:dataSource
  * tracker-miner-fs: Use low priority for update_async calls
  * tracker-search: New --software and --software-categories options

Translations:

  * Updated cs: Marek Cernocky
  * Updated de: Mario Blättermann
  * Updated es: Daniel Mustieles
  * Updated hu: Gabor Kelemen
  * Updated pt_BR: Djavan Fagundes
  * Updated sl: Andrej Žnidaršič

Notes:

  None


NEW in 0.9.28 - 2010-11-11
==========================

The changes are:

  * Require Vala 0.11.1
  * Require GLib 2.26
  * Require Automake 1.11 and use Vala features
  * NEW feature: GDBus support
  * New feature: Added utils/tracker-sql to run SQL on the databases directly
  * Fixes: GB#633118, GIF extractor doesn't extract comments
  * Fixes: NB#202275, Cardinality change from 1 to N in the ontology is not working fine
  * Fixes: NB#199968, Use gdbus in Vala to improve communications in Tracker
  * Fixes: NB#202627, Cannot create backup while updates are being inserted in the journal
  * Fixes: NB#202581, After backup restore operation: Tracker-CRITICAL **: tracker_class_has_insert_events: assertion `TRACKER_IS_CLASS (class)' failed
  * Fixes: NB#201122, Tracker store segfaults during backup Restore operation
  * Fixes: NB#203632, Remove rdfs:subClassOf from a class is not working
  * build: Fixed check for tracker-preferences which used "text" not "test"
  * Ontology: Updated mfo:image for mfo:FeedSettings to be in domain mfo:FeedElement
  * Ontology: Added tracker:indexed true for nmm:artistName
  * Ontology: Added tracker:domainIndex for nie:title on nmm:MusicAlbum
  * Ontology: Deprecated nmm:albumTitle for nie:title
  * functional-tests: Fixed and enabled property range date->string test
  * functional-tests: Fixed expected failures with new journal
  * functional-tests: Added property promotion/relegation tests
  * functional-tests: Added properties in basic ontology to test new changes
  * functional-tests: Added changing the superclass of a class test
  * libtracker-data: Combine INSERT and UPDATE statements into one INSERT
  * libtracker-data: Fixed ontology change coping for tracker:domainIndex
  * libtracker-miner: Added documentation for _manager_reindex_by_mimetype() and _manager_index_file()
  * tracker-store: Queue queries while backup is taking place

Translations:

  * Added pa: A S Alam
  * Updated cs: Marek Cernocky
  * Updated sl: Andrej Žnidaršič
  * Updated ca@valencia: Carles Ferrando
  * Updated es: Jorge González
  * Updated de: Mario Blättermann
  * Updated fr: Bruno Brouard

Notes:

  None


NEW in 0.9.27 - 2010-10-21
==========================

The changes are:

  * NEW feature: New journal format to separate ontology from data for improved data integrity
  * Fixes: NB#199131, Geotags are not displayed for the images
  * Fixes: NB#194908, Compilation albums (multiple albumArtist in the same Album) doesn't work properly
  * Fixes: NB#200310, BMP files should also be registered as nmm:Photo
  * Fixes: GB#633118, Read comments embedded in GIF files
  * Fixes: GB#633093, Use statvfs64() if available to check for free space
  * Fixes: GB#631391, MsOffice-XML content extraction doesn't work properly
  * build: Don't check for 'tar' command
  * functional-tests: Moved all IPC tests to ipc/
  * libtracker-common: Use per-client timeouts to avoid unnecessary wake ups
  * libtracker-client: Disable deprecation warnings when building this library
  * libtracker-data: Identify unsupported ontology change coping more broadly
  * libtracker-bus: Disable dbus timeouts in updates to the store
  * tracker-sparql: Added --list-indexes to show database indexes 
  * tracker-store: Use per-query watchdogs to avoid unnecessary wake ups
  * tracker-extract: Removed all tests, they've been commented out for some time
  * tracker-extract: GIF: Fixed memory leak for height and width
  * tracker-extract: GStreamer: Write multiple nmm:albumArtist if there are more than one
  * tracker-extract: MSOffice: Don't use a static var to check if nie:generator was already set
  * tracker-miner-fs: Removed all tests, test source file was just a main()
  * tracker-control: Fix runtime critical when journal file doesn't exist

Translations:

  * de: Mario Blättermann
  * sl: Andrej Žnidaršič
  * fr: Bruno Brouard

Notes:

  None


NEW in 0.9.26 - 2010-10-21
==========================

The changes are:

  * NEW feature: Task pool in libtracker-miner for store updates/metadata extractions, 10-20% speed increase indexing thanks to the work from Zhenqiang Chen
  * Fixes: GB#629850, Undefined reference errors during build
  * Fixes: GB#632399, [PATCH] configure.ac: Skip `AC_CHECK_FILE` when cross compiling
  * Fixes: NB#194548, Property needed to link nco:Contact with ncal:Event
  * build: Distribute .in.in files since they're needed for dist $prefix != target $prefix
  * build: Fixed warning with recent autoconf versions for AC_INIT
  * build: Fixed POTFILES.in, missing miner desktop files (was .in, not .in.in)
  * Ontology: Derive from rdfs:Resource for nao:Property, nfo:Orientation, poi:ObjectOfInterest, scal:AccessLevel, nmm:Flash, nmm:MeteringMode, nmm:WhiteBalance, nmm:RadioModulation, mto:TransferMethod
  * functional-tests: Test adding/removing properties in the ontology
  * functional-tests: Test adding/removing classes in the ontology
  * functional-tests: Rebased test-ontologies to latest data/ontologies
  * functional-tests: Removed sleep() calls, since we use D-Bus Wait() methods anyway, improves test speeds.
  * tracker-extract: Port QuillImageFilter usage to QImage
  * libtracker-data: Notify locales being used also for libunistring
  * libtracker-data: Improved collation related debugging traces
  * libtracker-bus: Handle error properly in array_update_finish()
  * libtracker-bus: Use host endian byte order for updates in steroids interface
  * libtracker-bus: Fixed memory leak when updating, GAsyncResult should always be unref-ed
  * libtracker-miner: Fixed critical warning when calling tracker-status -i
  * tracker-miner-fs: Fixed MeeGo app miner, missing include for GStrv and glib collators
  * utils, mtp-dummy: Fixed missing locale initialization

Translations:

  * ja: Takayuki KUSANO
  * es: Jorge González
  * et: Mattias Põldaru
  * nb: Kjartan Maraas

Notes:

  None


NEW in 0.9.25 - 2010-10-14
==========================

The changes are:

  * NEW feature: New API methods in libtracker-sparql which allow multiple inserts per DBus call (and yet get multiple errors back).
  * Fixes: NB#186096, X-MeeGo-logical-id not extracted from application .desktop files
  * Ontology: NMM: Set maximum cardinality of dlnaProfile and uPnPShared to 1
  * Ontology: NCO: Set tracker:notify for nco:ContactGroup.
  * SPARQL: Fix build with Vala < 0.11
  * SPARQL: Fix context handling for EXISTS and NOT EXISTS
  * SPARQL: Do not use pointers as keys in hash tables
  * SPARQL: Do not let SPARQL variable names influence SQL query
  * libtracker-sparql: Use lazy resolving for symbols in plugins
  * libtracker-sparql: Added missing APIs to documentation sections file
  * libtracker-sparql: Make new example for_update_array a bit more aesthetic
  * libtracker-sparql: Documented the new API methods for multi-insert
  * libtracker-data: Progress handling while performing integrity check
  * libtracker-data: Do not load superproperties on read-only access
  * libtracker-data: Avoid opening SQLite database twice on startup
  * libtracker-data: Force reindex on unsupported ontology change
  * libtracker-data: Fixed test cases on Fedora 14
  * tracker-store: Fixed status reporting
  * tracker-extract: Replace quill with quillimagefilter
  * tracker-extract: Use posix_fadvise() when closing files
  * tracker-extract: gupnp-dlna: Fix to use the tracker_sparql_escape_uri_printf()
  * functional-tests: Tests for tracker:indexed property
  * functional-tests: Tests for enable/disable notifications
  * functional-tests: Tests for cardinality changes in the ontology
  * functional-tests: Tests for first ontology-changes support
  * functional-tests: Added closing slash in aegis manifest file generator
  * functional-tests: Recovered tracker_store_remove_dbs() method. Needed for a backup test.
  * functional-tests: Fixed extraction expectations for GIF with XMP
  * functional-tests: Set PNG writeback test as expected failure until itxt is enabled in libpng (NB#185070)
  * functional-tests: Reliable start/stop of the store. Updated Backup/Journal tests
  * functional-tests: Logs improved.
  * functional-tests: Allow to change the ontologies directory in the system abstraction
  * functional-tests: Ignore Common* and *Template test classes in tests.xml
  * functional-tests: Added performance test for multi-insert
  * functional-tests: Ignore tracker-tests.aegis file generated during tests
  * coverage: Ignore coverage/ directory
  * utils: mtp-dummy: Implemented renaming of all files after batch copy finishes

Translations:

  None

Notes:

  None


NEW in 0.9.24 - 2010-10-07
==========================

The changes are:

  * Depend on automake 1.11 for Vala support amongst things
  * Depend on SQLite being compiled threadsafe (was previously a warning)
  * Build: Don't fail to build if Evolution headers are absent
  * Fixes: NB#195394, Adding a favorite tag column to a song query makes the ordering or results case sensitive, when sorted by title
  * Fixes: NB#196143, tracker-info core dumps with NULL cursor getting prefixes
  * Fixes: NB#195547, Corrupted DB prevents tracker-store from booting
  * Fixes: NB#192790, Music: Character (œ) is not supported in MP3 file
  * Ontology: Added comments to document more NMO classes and properties
  * Ontology: Added nmo:PhoneMessage, nmo:PhoneMessageFolder classes
  * Ontology: Added nmo:mmsHasContent property for nmo:MMSMessage
  * Ontology: Added predefined nmo:PhoneMessageFolder items for inbox, outbox, draft, sent, myfolder
  * Ontology: Changed domain to nmo:PhoneMessage for nmo:encoding, nmo:fromVCard, nmo:toVCard
  * Ontolgoy: Changed subclass to nmo:PhoneMessage for nmo:MMSMessage
  * Ontology: Deprecated nmo:containsSMSFolder, nmo:containsSMS, nmo:smsFolderId
  * SPARQL: Use COLLATE in SQL for variables and property functions
  * SPARQL: Use named graph of WITH clause in WHERE pattern for updates
  * functional-tests: Added verbosity on errors.
  * functional-tests: Don't start manually the store, let the miner-fs invoke it
  * functional-tests: Wait for the store before starting the tests
  * functional-tests: Create AEGIS security file at build time
  * libtracker-data: Add test case for ORDER BY property function
  * libtracker-miner: Make vapi file compatible with Vala 0.11.0
  * libtracker-sparql: Assume en_US.utf8_locale for test cases for proper collation
  * libtracker-sparql: Support GRAPH in SPARQL builder API
  * tracker-miner-fs: Fixed regression when deleting nfo:FileDataObject resources for a mount point
  * utils: mtp-dummy: Added a test case to synthesis an mtp daemon
  * plugins: Evolution: Fixed build error with updated tracker_sparql_connection*() API

Translations:

  None

Notes:

  The database version has been incremented, this will force a
  reindex for any existing Tracker installation.


NEW in 0.9.23 - 2010-09-30
==========================

The changes are:

  * Fixes: NB#193061, Music; §qtn_musi_comments_md§ string is indicated incorrectly in file details for MP3 files.
  * Fixes: NB#193044, Device doesn't do file list updates after uploading to device or delete from device procedures.
  * Fixes: NB#194746, DELETE statement fails with SQL error when GRAPH pattern is used
  * Ontology: Added nco:AuthorizationStatus class
  * Ontology: Set tracker:notify for nmm:MusicAlbum
  * Ontology: Set tracker:notify for nmm:Artist
  * Ontology: Set tracker:notify for nmm:ImageList
  * SPARQL: Fix SQL for GRAPH with predicate variable and empty database
  * coverage: ignore dbus-binding-tool generated files
  * functional-tests: Parse correctly playlist output from the extractor
  * functional-tests: Include playlists tests if totem-pl-parser is enabled
  * functional-tests: Added GIF with XMP metadata
  * libtracker-data: Log failure to set journal mode to WAL
  * libtracker-data: Fixed unchecked pointer unref when setting journal_mode pragma
  * libtracker-data: Change SQLite page and cache size
  * libtracker-sparql: Added connection_get_{async|direct_async}() APIs
  * libtracker-sparql: Various documentation fixes and additions.
  * libtracker-miner: Added new unit tests for blacklisting and event merging
  * libtracker-miner: Don't create inner loop until needed
  * tracker-store: Fixed corruption in FD passing D-Bus implementation
  * tracker-extract: MP3: Fixed a huge memory leak in albumart data
  * tracker-extract: MP3: Support Windows-1252 character encoding
  * tracker-extract: MP3: Fix string handling in comment and picture tags
  * tracker-extract: Album art: Stop looking for cover if we already
  * tracker-search-tool: Fixed compilation error with the latest Vala
  * plugins: Evolution: Fixed the UID format of the URL

Translations:

  * Updated pl: Piotr Drąg

Notes:

  None


NEW in 0.9.22 - 2010-09-23
==========================

The changes are:

  * NEW Feature: LRU for database statement cache (huge memory footprint difference)
  * NEW Feature: Monitor event black listing and events are condensed (less event spam up the stack)
  * Fixes: NB#192537, Insane memory usage in tracker-store
  * Fixes: NB#193796, tracker-extract crashes when extracting audio files due to an integration issue with Quill
  * Fixes: GB#630176, Build failure with GTK 2.90.7 (no GTK_DIALOG_NO_SEPARATOR available anymore)
  * SPARQL: Rework variable name support to fix regression
  * Ontology: NCO: Set tracker:notify to true for nco:ContactList
  * data-generators: Added a default value for undefined categories
  * functional-tests: Added collation tests
  * functional-tests: Set exec permissions to stats test
  * libtracker-data: Fixed regression SELECT ... AS ?var not being honored in ORDER BY
  * libtracker-data: Don't cache stmt if the query contains a regular expression
  * libtracker-data: Fixed memory leak in journal rotation
  * libtracker-sparql: Make it possible to set the max LRU cache size per connection
  * libtracker-sparql: Added an "environment variables" section to documentation
  * tracker-store: Added TRACKER_STORE_{SELECT|UPDATE}_CACHE_SIZE environment variables to control new LRU feature
  * tracker-store: Fixed crash when accessing variable_names array
  * tracker-extract: remove unneeded call to tracker_topanalyzer_shutdown()
  * tracker-extract: Fixed QUILLimagefilter support
  * tracker-miner-fs: Fixed use of uninitialized variable

Translations:

  * Updated et: Mattias Põldaru

Notes:

  None


NEW in 0.9.21 - 2010-09-16
==========================

The changes are:

  * Require D-Bus 1.3.1 or higher
  * NEW Feature: Return types in TrackerSparqlCursor API (previously just strings)
  * Fixes: NB#186055, Please make nco:note multi valued
  * Fixes: NB#192326, Tracker-store pollutes syslog with debug messages which are mostly only useful for tracker devs
  * Fixes: NB#188031, Tracker-store and tracker-miner-fs are listening to all serviceOwnerChanged messages on DBus  * Ontology: Added nfo:HelpDocument
  * Ontology: Added brief description to the ontologies documentation index page
  * functional-tests: Fixed signal tests by using arg0 correctly to get the right signal
  * functional-tests: Added tests for the stats API
  * functional-tests: Updated tests for the signals API
  * libtracker-fts: Do not use TLS for queries
  * libtracker-common: Reset default log handler in tracker_log_shutdown()
  * libtracker-data: Fixed parallel cursor_next_async() call segfaults
  * libtracker-miner: Removed Pango requirement in tracker-miner.pc
  * libtracker-miner: Improved debugging to avoid store availability confusion in log
  * libtracker-sparql: Added test case for cursor_next_async() segfaults
  * tracker-miner-flickr: Fixed desktop file not having the correct prefix for its icon
  * tracker-extract: On shutdown a summary is logged with extraction/failure count per module
  * tracker-extract: Fixed compilation error for quill support
  * tracker-extract: Don't extract content if config has max bytes set to 0
  * tracker-extract: Use g_pattern_match() instead of g_pattern_match_simple(), ~5% speed increase
  * tracker-search: Do not use tracker:available for emails
  * tracker-status: Fixed memory leak when printing state

Translations:

  * Updated hu: Gabor Kelemen

Notes:

  The database version has been incremented, this will force a
  reindex for any existing Tracker installation.


NEW in 0.9.20 - 2010-09-09
==========================

The changes are:

  * Fixes GB#628859, Ship examples/rss-reader/initial-data.ttl in tarballs
  * Fixes GB#628872, tracker-control -t or -k shouldn't kill tracker-status-icon
  * Fixes NB#187558, nfo:belongsToContainer not set for music files synchronized from Ovi Suite to the device
  * Fixes NB#189896, BatchCommit doesn't send reply
  * Ontology: Added slo:categoryIconUrl for category icons in the location ontology
  * Ontology: Added slo:iconUrl for landmark icons in the location ontology
  * Ontology: Added nfo:manufacturer to replace nfo:make
  * Ontology: Updated documentation, use plainTextContent instead of plainTextMessageContent
  * libtracker-data: Fixed signed char comparison in SPARQL scanner
  * tracker-store: Fixed class-signaling rdf_types array after delete
  * tracker-store: Fixed class-signaling to emit all triples for rdf:type predicates
  * tracker-extract: MSOffice, fixed to set nfo:Presentation & nfo:Spreadsheet
  * tracker-extract: GStreamer, use integer division for rounding duration
  * tracker-extract: Tiff, updated to use canonical uris with new nfo:manufacturer ontology
  * tracker-miner-flickr: Fixed so tracker-search-icon doesn't show constant polling
  * tracker-miner-flickr: Fixed criticals for photos with no tags
  * tracker-miner-rss: Fixed so tracker-search-icon doesn't show constant polling

Translations:

  * Updated de: Christian Kirbach
  * Updated se: Daniel Nylander

Notes:

  None


NEW in 0.9.19 - 2010-09-02
==========================

The changes are:

  * Require libicu or libunistring (GLib unicode support has performance issues)
  * NEW Feature: Collation support
  * NEW Feature: Refactored signal notification API
  * Fixes: NB#184505, Video files without audio are listed as music clips.
  * Fixes: NB#179465, Cannot sort lower and upper cases for under same letter
  * Fixes: GB#628535, "make check" fails with LDFLAGS=-Wl,--as-needed -Wl,-Bsymbolic
  * Fixes: GB#628536, Don't install ui files for example utils
  * Ontology: Added nmo:isEmergency property
  * functional-tests: Avoid FTS tests if FTS is disabled
  * functional-tests: Don't include TTL files in dist (tarball was 26Mb vs. 6Mb)
  * libtracker-data: Fixed memory leak when setting journal_mode pragma
  * libtracker-miner: If CREATED event is received from a monitor, force IRI cache update
  * libtracker-miner: Avoid gtk-doc warnings: multiple IDs for constraint linkend
  * libtracker-miner: Don't print paths as they may not be in UTF-8, print URIs
  * tracker-extract: Fixed xine, mplayer and totem backends
  * tracker-miner-fs: Removed CacheTimeout option from config, unused
  * tracker-miner-fs: Removed ScanTimeout from config, unused
  * tracker-miner-fs: Disable crawling entirely if !initial_crawl
  * tracker-control: Avoid compiler warning
  * ttl_sgml: Avoid compiler warning

Translations:

  * Updated cs: Marek Černocký
  * Updated sl: Andrej Žnidaršič

Notes:

  The unicode parser will no longer default to GLib and configure will
  error if no libicu or libunistring is available. This is due to
  serious performance issues that are witnessed using GLib. It can
  still be used if explicitly forced using the switch
  --with-unicode-support=glib


NEW in 0.9.18 - 2010-08-26
==========================

The changes are:

  * Fixes: GB#613252, Application-driven indexing and metadata storage
  * Fixes: GB#627893, tracker 0.9.17 does not compile with e-d-s/evolution 2.31.90 built with gtk+ 3.0
  * Fixes: NB#187148, "Not tagged, file is not indexed" message is seen when adding a tag
  * Fixes: NB#184505, Video files without audio are listed as music clips.
  * coverage: Ignore deprecated libtracker-client
  * coverage: Ignore generated .c files
  * functional-tests: Use python instead of specific python2.6
  * functional-tests: Corrupted-image extraction test is no longer an expected failure, as bug was fixed
  * functional-tests: FTS: Improve offsets/rank tests
  * functional-tests, FTS: Remove rdfs:Resource, not rdf:Resource
  * Ontology: Renamed slo:longitudeUncertainty to slo:horizontalAccuracy
  * Ontology: Renamed slo:latitudeUncertainty to slo:verticalAccuracy
  * libtracker-data: Fixed journal reader with rotated journals
  * libtracker-data: Make sure tracker_fts_update_commit() is called for FTS updates
  * libtracker-direct: Use maximum D-Bus timeout
  * libtracker-direct: Wait for tracker-store to be ready
  * libtracker-bus: Use maximum D-Bus timeout
  * libtracker-bus: Fixed linking to include libtracker-sparql
  * libtracker-bus: Throw errors for non-FD passing when Cancellable is cancelled
  * libtracker-bus: Support BatchUpdate for low priority requests
  * libtracker-sparql: Added debugging to know what SPARQL is used with direct-access
  * libtracker-sparql: Added TRACKER_SPARQL_BACKEND environment variable
  * libtracker-sparql: Added deps file for tracker-sparql.vapi to distribution
  * libtracker-sparql: cursor_get_n_columns() should only be called after cursor_next()
  * libtracker-miner: Allow API to bypass config dir checks for user based indexing
  * libtracker-miner: Add method to reindex certain mimetypes
  * libtracker-miner: Add method to invoke tracker-miner-fs to index a file
  * tracker-store: Fixed race conditions with new 'Wait' d-bus interface
  * tracker-store: Fixed unregister_g_object() criticals
  * tracker-store: Small busy callback improvements, indexing speed increased
  * tracker-miner-fs: Remove --add-file option (now done using tracker-control -f)
  * tracker-miner-fs: Added debugging to know which file is requested to be indexed over D-Bus
  * tracker-miner-fs: Use tracker_dbus_request_failed() for error situations
  * tracker-miner-fs: Do not index dirs unconditionally when adding monitors, huge crawling performance improvement
  * tracker-miner-fs: Renamed Reindex D-Bus interface to Index
  * tracker-control: Make -f and -m outputs translatable
  * tracker-control: Print when reindexing mime types or files is successful
  * tracker-control: Don't print PIDs if we're reindexing mimes or a file
  * tracker-control: Use TrackerMinerManager to reindex mimetypes
  * tracker-tag: Bail out if none of the passed files exist in the store

Translations:

  * Updated es: Jorge González
  * Updated fr: Bruno Brouard
  * Updated cs: Marek Cernocky
  * Updated de: Mario Blättermann
  * Updated sl: Andrej Žnidaršič

Notes:

  None


NEW in 0.9.17 - 2010-08-19
==========================

The changes are:

  * Require SQLite 3.7 (for Write Ahead Logging)
  * Require D-Bus GLib 0.82 (for dbus_g_connection_unregister_g_object())
  * Fixes: GB#627281, crash in tracker-0.8.15 ms-office extractor
  * Fixes: GB#623005, Remove odt2txt dependency in the OASIS extractor
  * Fixes: GB#623207, tracker 0.8.6/0.9.x fails to build against evolution 2.31.4
  * Fixes: NB#185950, COREWEB: /usr/lib/tracker/tracker-extract '*__GI_strncmp extract_gif get_file_metadata tra
  * Fixes: NB#185070, Enable PNG_iTXt_SUPPORTED flag in libpng
  * Fixes: NB#182948, Keywords/Subject metadata not written back to the file
  * Fixes: NB#184882, tracker-extract is getting crashed for malformed gif files
  * Functional Tests: refactoring branch merged, much more comprehensive testings now
  * SPARQL: Improve lexer performance for strings with ASCII characters
  * libtracker-common: Make sure libtracker-sparql inherits verbosity
  * libtracker-data: Merged DataError into TrackerSparqlError, was causing miner-fs to not finish
  * libtracker-data: Remove unused Options database table
  * libtracker-sparql: Added dbus-glib-1 dependency when building Vala sources
  * libtracker-sparql: Added example documentation for update_blank() API
  * libtracker-bus: Catch SPARQL errors thrown by D-Bus
  * tracker-extract: MP3, Fixed extraction of certain ID3v2.3 and ID3v2.4 tags
  * tracker-extract: MP3, Fixed un-unsynchronisation in ID3v2
  * tracker-store: Fixed some memory leaks in dbus shutdown
  * tracker-store: Removed plugin support (was only used by kmail)
  * tracker-miner-fs: Fixed memory leak when setting up volumes
  * tracker-search: Fixed typo, output was "Contacts:" when it should be "Files:"
  * tracker-search: Improved output for --contacts and --files
  * kmail: Removed this plugin, it was unmaintained

Translations:

  * Updated es: Jorge González

Notes:

  None


NEW in 0.9.16 - 2010-08-12
==========================

The changes are:

  * Require Vala 0.9.4 (to build from git repository only)
  * NEW Feature: tracker-extract: Added a GUPnP DLNA extractor
  * NEW Feature: libtracker-sparql: Client library for direct/dbus access
  * Fixes: NB#184823, Unsupported ontology change prevent tracker for booting
  * Fixes: NB#177663, Tracker does not harvest ID3v2 tags
  * Fixes: NB#181421, Duplicates when monitor events processed before directory is created in store
  * libtracker-common: New environment variable TRACKER_VERBOSITY to override config
  * libtracker-data: Fixed support for IN, NOT IN with <resources>
  * libtracker-miner: If parent GFile being processed, serialise by waiting
  * libtracker-miner: Print URIs not paths for CREATE monitor event debugging

Translations:

  * Updated zh_CN: Aron Xu
  * Updated nds: Nils-Christoph Fiedler

Notes:

  The database version has been incremented, this will force a
  reindex for any existing Tracker installation.


NEW in 0.9.15 - 2010-08-05
==========================

The changes are:

  * Require GLib 2.24
  * Fixes: NB#172390, Tracker doesn't return any content from m3u playlists
  * Fixes: NB#179570, FTS search of file extension doesn't always work
  * Fixes: NB#180803, Busy-handling race condition
  * Fixes: NB#181112, Fix GStreamer pipeline error handling
  * Fixes: NB#181538, Assume tar program is in $PATH
  * Fixes: NB#182131, Tracker isn't using resource tokens to protect its interfaces
  * Fixes: NB#182788, segfault during opendir()
  * Fixes: NB#183307, Crash in quit_timeout_cb when signal_handler calls it
  * Fixes: NB#183653, Class notification signals are lost
  * Ontology, MTP: Fix mtp:hidden
  * libtracker-data: Support for SPARQL 1.1's IN operator
  * libtracker-data: Implement progress indication for rotated journals
  * tracker-store: Queue queries while busy
  * Add first version of FTS functional test cases
  * Add functional test case for busy-handling

Translations:
  * Updated hu: Gabor Kelemen
  * Updated nds: Nils-Christoph Fiedler
  * Updated se: Daniel Nylander
  * Updated sl: Andrej Žnidaršič

Notes:

  The database version has been incremented, this will force a
  reindex for any existing Tracker installation.


NEW in 0.9.14 - 2010-07-23
==========================

The changes are:

  * Fixes: GB#616199, tracker-extract: Avoid second nie:title in msoffice/xml and oasis documents.
  * Fixes: GB#624731, libtracker-miner: Don't emit signals while iterating hash table.
  * Fixes: GB#624747, NB#179434: Avoid double-free in send_and_splice_async_callback()
  * Fixes: GB#624774, Fixes memleaks in tracker-miner-fs
  * Fixes: NB#180679, Make sure DBusGProxy is destroyed when destroying miner
  * Fixes: NB#180236, Filter not removed when going into UNAVAILABLE
  * Fixes: NB#179894, Reset sqlite3 stmt if detected as expired, and retry
  * Fixes: NB#181538, Assume tar program is in $PATH
  * NEW Feature: SPARQL, Add support for EXISTS and NOT EXISTS
  * Ontology, NAO: Set maximum cardinality of propertyName and propertyValue to 1
  * Ontology, maemo: Add contactGroup{Audio,Video,Vibration}Ringtone
  * libtracker-data: Avoid reading already disposed memory
  * libtracker-data: Improve logging after sqlite error, before aborting
  * libtracker-data: Remove the unused contents.db
  * libtracker-data: Remove storing of local locale for no reason
  * libtracker-data: Do not cope with ontology changes in read-only mode
  * libtracker-data: Do not reindex in read-only mode
  * libtracker-data: Do not fetch resource count in read-only mode
  * libtracker-data: Replace interrupt_thread by GCancellable
  * libtracker-db, journal: Fix static code analysis issues
  * libtracker-db: Merged into libtracker-data
  * libtracker-db tests: Merged into libtracker-data tests
  * libtracker-common: Fix double free when error splicing
  * libtracker-common: Fix memory leaks when receiving partial reply plus errors in DBus calls
  * libtracker-common: Fixed memleak in path_has_write_access()
  * libtracker-client: Avoid critical, file may not have parent
  * libtracker-client: Fix memleak, unref DBusPendingCall missing in notify callback
  * libtracker-client: Avoid compiler warning when using FD-passing
  * libtracker-client: Fix double free, error is freed in callback
  * libtracker-miner: Exclude some files from coverage reports
  * libtracker-miner: Fixed memleak in item_move()
  * libtracker-miner: Fixed memleak in mount_guess_content_type()
  * tracker-extract: Fix crash in albumart when pixbuf cannot be created
  * tracker-extract, msoffice: Fixed memleak in extract_msoffice()
  * tracker-miner-applications: Fix memleak in process_file_cb()
  * tracker-writeback: Fixed memleak in get_playlist_type()

Translations:
  * Updated sl: Andrej Žnidaršič
  * Updated es: Jorge González
  * Updated zh_CN: Jessica Ban

Notes:

  None


NEW in 0.9.13 - 2010-07-16
==========================

The changes are:

  * Fixes: GB#624071, t-s-t: Query results is always a 2 dimensional array.
  * Fixes: GB#623836, Avoid crashing if table of pieces is empty in the msword file
  * Fixes: NB#176090, libtracker-miner: Don't use same parent GFile for both mtime and iri caches
  * Fixes: NB#178825, New --feeds option in tracker-search to look for feeds
  * Fixes: NB#178808, Default verbosity should be errors-only in tracker-store
  * Fixes: NB#179559, Added domainIndex on nie:title for nmm:MusicPiece
  * Fixes: NB#179798, Wrong detection of ignored directory as a directory
  * Fixes: NB#178599, extra checks for finding the problem
  * Fixes: NB#174590, NCO: Remove cardinality restriction in nco:title
  * FixeS: NB#180602, Cannot detect is a song in device's local memory or not
  * NEW Feature: Added domain specific indexes, adding a column to DB tables to avoid full table scans/joins with larger tables for performance.
  * NEW Feature: Journal compression and rotation using 50Mb chunk sizes
  * libtracker-fts: Fix NEAR/XX operator with more than 2 digits
  * libtracker-fts: Useful debugging log included, disabled by default
  * libtracker-fts: Avoid possible NULL dereference
  * libtracker-fts: Initialize all buffers before using them
  * libtracker-fts: Avoid segfault when using the NEAR/N operator in FTS
  * libtracker-data: Added important debugging when changing database (drops/alters/creates/etc)
  * libtracker-data: Call g_error() if getting max ID fails
  * libtracker-miner: Improve logging when duplicates are found for a given uri
  * libtracker-miner: Improved logging, instead of "Adding item", now uses "Creating"/"Updating"
  * libtracker-miner: Improve logging for storage reporting
  * libtracker-miner: Avoid unused variable warnings in test cases
  * libtracker-client: Fix double free in error handling
  * libtracker-client: Fix object_date proto in vapi
  * libtracker-client: Mark graph as nullable in vapi
  * tracker-miner-fs: Use 60s as default DBus timeout for requests to extractor
  * tracker-extract: MSOffice-xml: Avoid possible NULL dereference
  * tracker-extract: PDF: Avoid possible NULL dereference
  * tracker-extract: Avoid warnings when compiling C++ code with C-only warning flags

Translations:

  * Updated de: Mario Blättermann
  * Updated pt: Nicolau Gonçalves
  * Updated cs: Marek Cernocky

Notes:

  None


NEW in 0.9.12 - 2010-07-08
==========================

The changes are:

 * Fixes: NB#175613, Avoid duplicate media files showing in databases when sending MMS'
 * Require GLib 2.24
 * Removed unused PKGLIBDIR defines in Makefile.ams
 * Removed libinotify imported module and use GIO completely
 * libtracker-common: Added common functionality for D-Bus FD passing with send_and_splice API
 * libtracker-common: Avoid unused variable warnings in unit tests
 * libtracker-common: Always set last_time in D-Bus client data
 * libtracker-db: Remove unreachable code
 * libtracker-db: Avoid results->GSList->GStrv in _dbus_query_result_to_ptr_array(), just use ->GStrv
 * libtracker-fts: initialize all buffers before using them
 * libtracker-fts: Avoid unneeded check, rc is always SQLITE_OK
 * libtracker-fts  Remove unused variable in test case
 * libtracker-miner: If mount has no volume, don't rely on is_blank for guessing content type
 * libtracker-miner: Improve logging for storage module
 * libtracker-miner: Added tracker-monitor test cases
 * libtracker-miner: Avoid dead code in mount_guess_content_type()
 * libtracker-miner: Avoid unused variable warning in unit test
 * libtracker-client: Fixed object_date bindings in VAPI file
 * libtracker-client: Mark graph as nullable in VAPI file
 * tracker-store: Fix crash when not building with glibc
 * tracker-store: Avoid potential pointer ptr->reply dereference where ptr is NULL
 * tracker-store: Avoid unused variable warning
 * tracker-extract: Avoid unreachable code when not using libstreamanalyzer
 * tracker-extract: MP3: avoid deadcode, bitrate will always be greater than 0
 * tracker-extract: PDF: check if document is returned before dereferencing it
 * tracker-extract: Implement tracker_dbus_send_and_splice() for miner/extractor communications
 * tracker-writeback: Avoid unused variable warning

Translations:

  * Updated de: Mario Blättermann
  * Updated sl: Andrej Žnidaršič

Notes:

  None


NEW in 0.9.11 - 2010-07-01
==========================

The changes are:

 * Fixes: GB#511474, Remove the option to index remote mounted directories
 * Fixes: GB#622734, too many open files error / possible file descriptor leak
 * Fixes: GB#623203, Fix segfault when guessing content type in mounts without path
 * Fixes: GB#616809, t-s-t shows Author as urn:uuid: in metadata bar
 * Fixes: GB#622702, 0.8.13 does not build with gio 2.22 anymore
 * Fixes: NB#174202, Some .pya audio files cannot be displayed in Music application after copying to device via USB.
 * Fixes: NB#176932, Pure asf audio file is displayed in Video&TV application.
 * Fixes: NB#177447, Remove the 'error' signal from TrackerMiner object
 * general: Added -Wno-portability to AM_INIT_AUTOMAKE
 * general: Bumped version requirement for GLib from 2.20 to 2.24
 * general: Bumped version requirement for D-Bus from 1.3.0 to 1.3.1 for FD passing
 * tracker-control: Fixed typo in --help
 * tracker-extract: Plug leak in JPEG extractor
 * tracker-extract: Plug leaks in MSOffice extractor
 * tracker-extract: Plug leaks in PDF extractor
 * tracker-extract: Plug leaks in XMP extractor
 * tracker-extract: Plug leaks in GStreamer extractor on pipeline creation errors
 * tracker-store: Avoid CPU hopping of main threads (GNU only platforms)
 * tracker-store: Improve memory allocation for change events and avoid duplicates
 * tracker-store: Always reset interrupt thread before queries AND updates
 * tracker-store: Fix call to tracker_store_sparql_update for FD passing API
 * tracker-miner-fs: Use TrackerMinerFS caches for application mining
 * tracker-miner-fs: Fix various SPARQL issues for application mining
 * libtracker-data: Fix typo in define used for FTS support
 * libtracker-client: Updated Vala bindings
 * libtracker-miner: Fix potential invalid read error
 * libtracker-miner: Fix compilation if UPOWER and HAL are disabled
 * evolution: Don't fail to add mails with spaces in the folder name
 * function-tests: added mass storage mode test cases

Translations:

  * Updated et: Mattias Põldaru
  * Updated de: Mario Blättermann
  * Updated es: Jorge González
  * Updated sl: Andrej Žnidaršič

Notes:

  None


NEW in 0.9.10 - 2010-06-24
==========================

The changes are:

  * Fixes: GB#621547, inconsistent messages on hard-reset
  * Fixes: GB#620354, make TST history persistent
  * Fixes: GB#621070, Re-enable volume cleanup
  * Fixes: GB#620828, returns no e-mail results
  * Fixes: GB#621987, Correctly handle applications in results
  * Fixes: GB#620362, mp3 extraction crashes if year string is NULL before using atoi() with libenca enabled
  * Fixes: GB#619868, don't recheck all monitored directories needlessly
  * Fixes: GB#620486, Ignored files can still be found after changing tracker-miner-fs.cfg
  * Fixes: GB#622351, Wrong initial detection of mounted state of the root partition
  * Fixes: NB#175723, Avoid duplicates if several monitor events arrive together and store is slow
  * Fixes: NB#170915, tracker-search for snippet is resulting in errors
  * Fixes: NB#175957, Properly setup non-removable mounts at startup
  * Fixes: NB#175712, tracker is listing the files under Ignore directories
  * Fixes: NB#172813, Tracker Miner polluting dbus channels upon indexing
  * Support --disable-tracker-fts to disable libtracker-fts and all FTS operations
  * Support D-Bus FD passing (avoids much memory copying) thanks to the work from Adrien Bustany
  * SPARQL: Fix generated SQL for triple block following simple optional
  * libtracker-data: Add tests for triple block following simple optional
  * libtracker-miner: Fixed memory leak
  * libtracker-miner: Minor style changes
  * tracker-miner-fs: Force recheck on IgnoredFiles changes
  * Revert index added for nfo:belongsToContainer
  * Revert "libtracker-data: Fixes NB#174591, Throw error on undefined variable in expressions"

Translations:

  None

Notes:

  Building Tracker from the git repository requires updated librest
  bindings not yet released. Building the Flickr miner requires master
  vala to be built. This can be disabled using --disable-miner-flickr.


NEW in 0.9.9 - 2010-06-17
=========================

The changes are:

  * Fixes: GB#621547, inconsistent messages on hard-reset
  * Fixes: GB#621126, Status icon's progress bars should be at 0% on init
  * Fixes: GB#620094, tracker-status-icon shows "search" menu item even though t-s-t is not installed
  * Fixes: GB#621183, When mounting volume 2nd time, inotify watches don't work any more
  * Fixes: GB#621001, tracker-miner-fs fails to initialize active mount points if CD name has whitespaces
  * Fixes: NB#174162, Tracker's filesize returns integer which prevents to show correct file size for big files.
  * Fixes: NB#174333, Each file is listed twice in tracker search results
  * Fixes: NB#174591, Tracker accepts unbound variables in FILTER() expression.
  * Fixes: NB#173673, SMS validity period property needed in nmo ontology
  * Fixes: NB#172818, Tracker is not indexing new content on EMMC
  * Fixes: NB#173589, tracker-store is killed due to high memory usage from large INSERTs
  * Fixes: NB#174248, Filtering by boolean value fails
  * configure: Fixed summary for miners (strings were blank sometimes)
  * Ontology: Removed nrl:maxCardinality from nco:hasAffiliation (was 1)
  * Ontology: Added tracker:isRemovable and tracker:isOptical properties
  * libtracker-db: Database version bump (due to ontology changes)
  * libtracker-db: Simplify DB abstraction, it is not needed
  * libtracker-db: Add a "length" parameter to tracker_db_cursor_string
  * libtracker-fts: Fixed libicu backend memory leak in tracker_parser_reset()
  * libtracker-miner: Added tracker_storage_get_type_for_uuid()
  * libtracker-miner: Update the database when config changes for optical/removable media
  * libtracker-miner: Use GMount instead of GVolume which is for user-mountable devices only
  * libtracker-miner: Detect removable devices using GDrive API, not guessing
  * libtracker-miner: Ignore GMounts which are considered shadowed mounts
  * libtracker-miner: Import GIO's system for detecting optical media more accurately
  * tracker-store: Optimized memory use for class signals
  * tracker-store: Optimized class signals with use of GStringChunk
  * tracker-store: Avoid using dbus-glib for sending class signals
  * tracker-store: Compress SubjectsChanged signal for multi-value fields
  * tracker-extract: Slightly reduce SPARQL update size for playlists
  * tracker-miner-fs: Force re-check of mounted path ONLY if an indexed location
  * tracker-minre-fs: Fixed build when there is no DEVKIT or HAL

Translations:

  None

Notes:

  The database version up has been incremented, this will force a
  reindex for any existing Tracker installation.


NEW in 0.9.8 - 2010-06-10
=========================

The changes are:

  * Fixes: NB#171657, Device manufacturer and device model XMP metadata are not extracted from mp4 video files
  * Fixes: NB#173206, Images and videos should both have the key 'nfo:device' with the same information
  * Fixes: NB#161449, Empty place holder in query result after copying new songs to device
  * Fixes: NB#172414, Song without any album image is shown with some random primary image in Content Manager
  * Fixes: NB#172652, Avoiding some missing playlists in query doesn't
  * return anything other exist playlists in query
  * Fixes --enable-maemo (had no effect)
  * Updated generator scripts to include data objects for emails
  * SPARQL: Warn instead of crash on namespaces without a prefix
  * SPARQL: Added test for FILTER in group graph patterns
  * SPARQL: Always generate SELECT around group graph patterns
  * SPARQL: Only clear buffer when rolling back a transaction
  * SPARQL: Support experimental INSERT SILENT and DELETE SILENT (don't rollback on failures during transactions)
  * Ontology: Added tracker:defaultValue property
  * Ontology: Added maemo:relevance default value of 1000000
  * Functional Tests: Added script to kill store during batch commit
  * libtracker-data: Fix error annotations in Vala bindings
  * tracker-miner-fs: Fixed IRI cache invalidation
  * tracker-miner-fs: Use DELETE instead of DROP GRAPH for volumes
  * tracker-miner-fs: Separate user data from filesystem data using graphs
  * libtracker-common: Removed public documentation for private library
  * libtracker-db: Renamed crawling-timestamp to last-crawl.txt
  * libtracker-db: Renamed .firstindex to first-index.txt
  * libtracker-db: Set contents of first-index.txt to PACKAGE_VERSION

Translations:

  * Updated sl: Andrej Žnidaršič
  * Updated hu: Gabor Kelemen
  * Updated es: Jorge González

Notes:

  None


NEW in 0.9.7 - 2010-06-03
=========================

Improvements / New:

  General:
  * Fixed _DATA include for Maemo ontologies, was missing \
  * Use simple contacts instead of PersonContacts for message data generators

  Functional Test:
  * Added experimental nmo:Message query tests

  Ontology:
  * Deprecated nmm:camera for nfo:device
  * Added nmo:ReportReadStatus class
  * Added nmo:sentWithReportRead property
  * Added nmo:reportReadStatus property
  * Added nmo:read-status-read, nmo:read-status-deleted and nmo:read-status-unknown for nmo:ReportReadStatus
  * Added nmo:mmsId property
  * Don't use nie:hasPart subproperty for nco:key, nco:photo, nco:logo and nco:sound

  libtracker-common:
  * Added TrackerConfigFile and TrackerKeyfileObject Vala bindings

  libtracker-fts:
  * Removed tracker_parser_process_word() API

  tracker-store:
  * Added support for TRACKER_STORE_TASK_MAX_TIME environment variable to set maximum time allowed before interrupting a query
  * Process 50 Turtle statements at once, previously was 10

  tracker-miner-fs:
  * Added 'CrawlingInterval' config option to disable or only mtime check file system after n days, default is same as previously (every startup)

  tracker-miner-rss:
  * Link against libtracker-client since we use the API

Bugs:

  * NB#167647 - Packages built from tracker depend on python2.5 but call /usr/bin/python
  * GB#619498 - Check if stop word before stemming, not after
  * GB#619929 - tracker-search shouldn't quit if stopword found
  * GB#619535 - MaxWords configuration in FTS config is not used

Translations:

  * Updated de: Mario Blättermann
  * Updated ja: Takeshi AIHANA

Notes:

  None


NEW in 0.9.6 - 2010-05-27
=========================

Improvements / New:

  General:
  * Fixed libtracker-client header includes for all tracker-utils

  Ontology:
  * Added SLO (Simplified Location Ontology) for testing

  libtracker-db:
  * Replaced unreliable sqlite3_interrupt() with another solution

  libtracker-data:
  * Fixed backup test to properly shutdown libtracker-data
  * Improved error reporting when inserting multiple values for single valued property
  * Don't remove temporary data asynchronously, leads to race conditions

  libtracker-extract:
  * XMP: Protect against division by zero
  * Updated tracker_text_validate_utf8(), text_len was gsize, now it is gssize

  libtracker-fts:
  * Make sure to not keep table locked after queries
  * Added README.parsers to explain libicu/libunistring/libglib parsing methods
  * Removed max_words_to_index, it was unused
  * Removed delimit_words, it was unused

  tracker-extract:
  * XMP: Fix strings that go like "n1/n2" that are xsd:double

  tracker-writeback:
  * XMP: Don't writeback fields that are typically set by the camera itself

  tracker-search:
  * Don't append "*" to all fts:match queries

Bugs:

  * GB#619826 - don't clean-up search terms before storing them in history
  * GB#526346 - please add option to disable stemming
  * GB#560220 - New option required:ignore diacritics when indexing and searching
  * GB#619271 - pango dependency only needed if compiling with the glib/pango parser
  * GB#619487 - ensure that one can search terms with _underscores_
  * GB#619630 - HTML extractor inserts several nie:title if title has an ampersand (&) character
  * GB#619615 - Include DC ontology in gtk-doc documentation
  * GB#619801 - We need SPARQL escaping for cases where we use g_file_info_get_display_name()
  * GB#616540 - tracker-preferences doesn't like non-UTF8 locale
  * NB#167449 - The attached PDF file is not displayed in the front page.
  * NB#168740 - Tracker-Critical **: Could not execute sparql: Unable to insert multiple values for subject
  * NB#169460 - Added m3u playlist is not harvested by tracker in SB1
  * NB#170142 - Same tags are extracted twice if found from DC:Subject and IPTC:keywords
  * NB#164822 - Need property to associate a video URL with a contact

Translations:

  * Updated fr: Claude Paroz

Notes:

  None


NEW in 0.9.5 - 2010-05-20
=========================

Improvements / New:

  General:
  * Added support for libunistring and libicu thanks for the work from Aleksander Morgado
  * Fixed building with VPATHS where $srcdir != $builddir

  SPARQL:
  * Added tracker:id function to return the integer ID used as last sort key in queries

  Ontology:
  * Added secondary index on nmo:communicationChannel

  libtracker-data:
  * Create new contexts per tripple now, improves variable handling
  * Added support for secondary indexes, creates a 2 column index in SQLite

  libtracker-fts:
  * Fixed word breaking (if using libicu or libunistring)
  * Fixed word normalization (if using libicu or libunistring)
  * Fixed word case folding (if using libicu or libunistring)
  * Support multi language extraction per file (if using libicu or libunistring)

  libtracker-miner:
  * Fixed IRI cache
  * Fixed mtime cache for config directories themselves
  * Added TrackerNetworkProvider for miners which need to know about connection status

  tracker-store:
  * Check idle handler after
  * Use exclusive thread for TTL imports, fixes remaining table locked issues

  tracker-extract:
  * Handle single null terminated strings in UCS2 and UTF-16 in mp3 extractor
  * Don't do word breaking to limit FTS extraction by words, instead use bytes, faster
  * Added specific GIF file extractor

  tracker-search-tool:
  * Fixed segfault when entering text while categories are selected

Bugs:

  * NB#163155 - tracker-extract is crashing while extracting album art of a mp3 file
  * NB#169254 - GLIB CRITICAL ** GLib - g_utf8_strdown: assertion `str != NULL' failed
  * NB#169105 - GLIB CRITICAL ** Tracker - tracker_iptc_new: assertion `len > 0' failed
  * NB#169119 - GLIB CRITICAL ** GLib-GObject - g_object_unref: assertion `G_IS_OBJECT (object)' failed
  * GB#619061 - add search history and auto-completion
  * GB#616535 - miner-fs stops after crawling a little (no crash). maybe non-utf8 locale related.
  * GB#614870 - Open t-s-t on left click
  * GB#614610 - config behaviour regarding xdg-user-dirs doesn't work properly
  * GB#618821 - Wrong X-GNOME-Bugzilla-Components for desktop files
  * GB#618544 - Only install tracker-search-bar man page when enabled
  * GB#618011 - UI problems with tracker tray icon
  * GB#616808 - dist tarball fails to build twice in a row if --enable-gtk-doc is selected
  * GB#491850 - Allow option to ignore stopwords
  * GB#503366 - Cannot search number in text files
  * GB#579756 - Unicode Normalization is broken in Indexer and/or Search
  * GB#616836 - Use libunistring's u8_normalize() instead of GLib's g_utf8_normalize()

Translations:

  * Updated cs: Petr Kovar
  * Updated de: Mario Blättermann
  * Updated sl: Andrej Žnidaršič

Notes:

  None


NEW in 0.9.4 - 2010-05-11
=========================

Improvements / New:

  General:
  * Require poppler 0.12.2 instead of poppler-glib 0.4.5
  * Removed many unused variables from coverity reports
  * Various other fixes picked up from coverity reports
  * RSS miner, pkg-config should look for rest-0.6, not rest

  Functional Tests:
  * Added force-sqlite-misused-batch.sh and force-sqlite-misused.sh for NB#166684
  * Added for journal replay
  * Added for backup-restore

  Ontology:
  * Fixed typo for nfo:softwareCmdLine comment
  * Added tracker:notify for nco:EmailAddress
  * Added tracker:notify for nco:PhoneNumber
  * Added tracker:notify for nco:PostalAddress
  * Added tracker:notify for nfo:Document
  * Added tracker:notify for nfo:SoftwareApplication
  * Added tracker:indexed for nfo:belongsToContainer

  Data Generators:
  * Fixed %u use since it is deprecated in favour of %d in Python

  libtracker-common:
  * Fixed compilation error in validating ints for tracker-keyfile-object

  libtracker-db:
  * Enable read_uncommitted to fix locking issues
  * Properly shut down thread data on thread destruction, fixes sqlite3_close() log errors
  * Don't fsync/close already open databases if g_open() succeeds
  * Avoid strstr in uri_is_{parent|descendant} functions
  * Performance improvement for tracker:uri-is-parent function

  libtracker-extract:
  * Don't modify setlocale() return value, as it's statically stored
  * Protect against invalid values in tracker_date_guess()

  libtracker-miner:
  * Added missing .deps file for Vala bindings
  * Fixed memory leak in VAPI file
  * Fixed includes for libtracker-client in VAPI file
  * Updated nfo:fileLastModified query to use nfo:belongsToContainer, much faster

  libtracker-client:
  * Fixed typo in documentation for tracker_resources_sparql_query()

  tracker-store:
  * Fixed possible memory corruption
  * Fixed crash when client disappears
  * Fixed unref of TrackerDBCursor when NULL
  * Handle commit transaction error when importing turtle files and rollback

  tracker-extract:
  * Ported PDF extractor to not use poppler-glib, speed improvement
  * Don't run past genre array in mp3 extractor
  * Use nfo:HtmlDocument instead of nfo:Document in html extractor
  * Fixed compilation warning for msoffice extractor, use G_GSIZE_FORMAT not %d

  tracker-writeback:
  * Added MP4 and 3GPP video files xmp module

  tracker-search:
  * Added --emails and list subjects/dates
  * Added --contacts and list names/addresses
  * Updated --detailed so we only report URNs if this is supplied

  tracker-tag:
  * Fixed possible use of uninitialised memory

  tracker-info:
  * Show results in shortened form, added --full-namespaces for old behaviour

  tracker-search-tool:
  * Fixed segmentation fault when there are no results
  * Added "Folders" category
  * Renamed "Office Documents" category to "Documents"

Bugs:

  * GB#616548 - DIrectory lists and de-duplication in tracker-preferences
  * GB#617082 - RFE Useability - Learning How to Start the Miners was a Little Unintuitive
  * NB#166767 - Every word in a long tag is indexed separately
  * NB#166684 - Database table is locked issue, SQLITE_MISUSED is reported SQLite error here
  * NB#167616 - Tracker's ISO 8601 timestamp implementation is not correct
  * NB#166717 - Only first DC:Subject and IPTC:Keywords tags are extracted

Translations:

  * Updated cs: Petr Kovar
  * Updated de: Mario Blättermann

Notes:

  Due to the HTML extractor changing the document type from
  nfo:Document to nfo:HtmlDocument, we would advise anyone with these
  files indexed to use:

    tracker-control --reindex-mime-type text/html

  This should make sure all indexed files are correctly adjusted.


NEW in 0.9.3 - 2010-04-29
=========================

Improvements / New:

  General:
  * Fixed miner desktop files so they have trailing ; for Categories
  * Added structural view design document
  * Added queries for contact data generation scripts
  * Added queries for message data generation scripts
  * Added debugging to libinotify to assist in fixing GB#615965
  * Updated data generation scripts and templates quite heavily
  * Disable inotify and tracker-storage for coverage reports

  Ontology:
  * Added nco:contactLocalUID property

  Functional Tests:
  * Fixed query used for getting image metdata
  * Fixed signal handling in miner file operations test
  * Fixed performance scripts to not use isStoredAs
  * Updated contact related performance queries
  * Updated message related performance queries
  * Added writeback support for PNG, TIFF, MP4

  libtracker-common:
  * Removed tracker_power_get_battery_percentage(), it's unused
  * Port from libdevkit-power-gobject to libupower-glib

  libtracker-db:
  * Added tracker:uri-is-parent SQLite functions (for crawling improvements)
  * Added tracker:uri-is-descendant SQLite functions (for crawling improvements)
  * Define a maxium size for each DBusMessage

  libtracker-data:
  * Added fn:lower-case function

  libtracker-miner:
  * Improve crawling queries (3693 dirs, 27678 files, was 651s, now 166s)

  libtracker-client:
  * Added initial test cases

  tracker-store:
  * Fixed cursor iteration to make sure it is done in the thread that prepared the query
  * Fixed memory corruption, using DBusGMethodInvocation after freed
  * Create DBusMessages without dbus-glib to avoid memory duplication/fragmentation

  tracker-extract:
  * Fixed man page for -d
  * Fixed double free in Vorbis extractor, caused timeouts in miner-fs logs
  * Added tracker_getline() API for platforms without it

  tracker-miner-fs:
  * Use new libupower API in tracker-common

  tracker-miner-flickr:
  * Added new miner for Flickr web service thanks to the work from Adrien Bustany

  tracker-writeback:
  * Fixed double free in taglib module

  evolution:
  * Fixed race condition

  tracker-search-tool:
  * Removed --service (old 0.6 option which is unused)
  * Added support for starting queries using command line arguments

Bugs:

  * GB#615930 - crash in evolution caused by tracker plugin
  * GB#616493 - Remove unzip dependency from the OASIS extractor
  * GB#615836 - tracker:available doesnt work: wrong dataSource for removeable drives
  * GB#579051 - provide getline() if not found
  * NB#166080 - extraction of copyright for jpg image is partially correct
  * NB#164958 - tracker-search for a default song doesn't work for "beautif" and "beautifu"

Translations:

  * Updated cs: Petr Kovar
  * Updated sl: Andrej Žnidaršič

Notes:

  None


NEW in 0.9.2 - 2010-04-22
=========================

Improvements / New:

  General:
  * Fix build failures on none Linux platforms
  * Depend on SQLite >= 3.6.17 for shared cache mode (for FTS)

  Ontology:
  * nfo:isContentEncrypted was defined in nmm, not nfo
  * Added tracker:indexed for nfo:fileLastModified
  * Added tracker:fulltextIndexed for nie:subject
  * Added tracker:fulltextNoLimit for nie:subject
  * Added tracker:weight of 5 for nie:subject
  * Added tracker:indexed for nmm:musicAlbum
  * Added tracker:indexed for nmm:performer
  * Added tracker:indexed for mlo:location

  Functional Tests:
  * Added videos to the data generation
  * Added test set configuration for maximum values
  * Added full text queries
  * Added basic file operations for miner-fs and desktop environments
  * Added file operation tests for multilevel subfolders
  * Fixed writeback tests to wait for signal before testing extraction

  libtracker-db:
  * Support O_LARGEFILE when using g_open for the journal
  * Support cancellation (after 30 seconds) for abusive queries
  * Use sqlite3_config (SQLITE_CONFIG_MULTITHREAD)

  libtracker-data:
  * Fixed memory leak on journal replay

  libtracker-miner:
  * Don't translate statuses
  * Improve crawling queries (3693 dirs, 27678 files, was 651s, now 166s)

  libtracker-extract:
  * Don't run past an array in XMP tests

  tracker-store:
  * Support all requests using GThreadPool
  * Support parallel queries (uses 2 threads for now, tested with more)
  * Support cancelling queries when clients disconnect (from D-Bus)

  tracker-extract:
  * Set nfo:isContentEncrypted for encrypted docs
  * Improve <script> bypassing.

  tracker-sparql:
  * Fixed typo in man page for command line args

  tracker-status:
  * Added --list-common-statuses option

  tracker-control:
  * Added --reindex-mime-type and --start options to man page docs

Bugs:

  * GB#615856 - add html content extraction
  * GB#615868 - Improved OASIS extractor
  * GB#616165 - Improved reading PS and PS-GZ files
  * GB#615948 - Improved reading msoffice/xml files
  * GB#616403 - Improve & fix reading msoffice/powerpoint files
  * GB#616329 - Improve and fix reading msoffice/excel files
  * GB#616158 - Improve reading msoffice files
  * NB#161964 - Some special audio files are not displayed in the Music & Radio application after copying to device using USB networking.

Translations:

  * Updated es: Jorge González
  * Updated sl: Andrej Žnidaršič
  * Updated de: Christian Kirbach

Notes:

  None


NEW in 0.9.1 - 2010-04-15
=========================

Improvements / New:

  General:
  * Fixed erroneous linking where GdkPixbuf, HAL, DeviceKit, Pango and UNAC were involved

  Ontology:
  * Added tracker:writeback for nmm:performer and nmm:musicAlbum
  * Added nfo:heading and nfo:tilt properties

  libtracker-common:
  * Fixed use of timegm on BSD and use it for __GLIBC__, it's faster

  libtracker-db:
  * Avoid type checking for TrackerDBInterface and TrackerDBResultSet

  libtracker-data:
  * Added ontology change support for properties
  * Added ontology change error reporting
  * Added ontology change support for tracker:notify, tracker:writeback and tracker:indexed
  * Avoid type checking for TrackerProperty, TrackerClass and TrackerNamespace

  libtracker-client:
  * Added API to connect to writeback D-Bus service
  * Added properties to Vala bindings (allowing property = value)

  libtracker-extract:
  * Added tracker_{xmp|iptc|exif}_{new|free}() functions
  * Added tracker_coalesce_strip() to replace tracker_coalesce()
  * Added tracker_merge_const() to replace tracker_merge()
  * Refactored memory usage in extractors by use of tracker_coalesce()

  tracker-store:
  * Use priority queues in preparation for async queue handling branch

  tracker-miner-fs:
  * Fixed build failures on non-Linux systems (FAT filesystem operations)

  tracker-miner-rss:
  * Fixed linking to build with GThread

  tracker-extract:
  * Fixed PNG extractor to use more recent libtracker-extract Exif/Xmp APIs

  tracker-writeback:
  * Replaced id3lib module (known to corrupt mp3s) for taglib module

  tracker-tag:
  * Support nao:description with --description when using --add

Bugs:

  * GB#615182 - Avoid initializing priorities twice in tracker-miner-fs
  * GB#615313 - Avoid Invalid Read when iterating INotifyHandle list
  * GB#615184 - Is the low memory configuration option deprecated?
  * GB#615563 - Proper handling of --enable-evolution-miner configure option
  * GB#615178 - Graceful crawling stop
  * GB#615035 - Removing unzip command dependency from ms-office extractor
  * NB#161047 - Images resolution is not extracted
  * NB#163544 - substring function needed
  * NB#163715 - Tracker doesn't handle e.g. "11"^^<http://www.w3.org/2001/XMLSchema#int> correctly
  * NB#163480 - Tracker does not seem to be reading all the tags associated with an image

Translations:

  * Updated cs: Petr Kovar
  * Updated lt: Gintautas Miliauskas
  * Updated sl: Andrej Žnidaršič

Notes:

  None


NEW in 0.9.0 - 2010-04-08
=========================

Improvements / New:

  General:
  * Updated README
  * Removed TODO
  * Set functional tests to be enabled by default
  * Set libunac to be enabled if available by default

  Functional Tests:
  * Improved performance-tc.py, mostly whitespace changes

  Ontology:
  * Added tracker:indexed for nmo:from
  * Added tracker:indexed for nmo:to

  libtracker-data:
  * Added tests for more than one regex query
  * Fixed SPARQL regex, don't use bound strings, use literals
  * Fixed memory leak due to reference cycle

  libtracker-miner:
  * Fixed debian builds, don't use $(builddir) in Makefile.am

  tracker-miner-rss:
  * Added missing D-Bus service file
  * Added logging to show what is going on
  * Link with libtracker-common
  * Small code clean ups

  tracker-preferences:
  * Added file chooser button for ignored directories
  * Remove separator in patterns dialog
  * Make OK button default action in patterns dialog
  * Set throttle/low disk space only when Apply button is clicked

  evolution:
  * Avoid e-d-s 2.28 #define causing compilation errors, see GB#613199

Bugs:

  * GB#614981, Make distcheck depend on important components
  * GB#614722, Strcasestr is defined on FreeBSD
  * GB#614877, Remove deprecated Encoding key from desktop files
  * GB#614560, Build problems in the gio file monitoring case.
  * NB#162546, tracker-miner-fs is emitting criticals after resuming...
  * NB#158962, tracker-miner-fs emits criticals for some desktop files

Translations:

  * Updated de: Mario Blättermann
  * Updated es: Jorge González

Notes:

  The database version up has been incremented, this will force a
  reindex for any existing Tracker installation.


NEW in 0.8.0 - 2010-04-01
=========================

Improvements / New:

  General:
  * Added data/ontologies/Indicies.list for db index reasoning
  * Added full content data generator for testing
  * Fixed ALL license headers using Debian licensecheck.pl script

  Ontology:
  * Set tracker:indexed to true for nco:hasIMAccount
  * Set tracker:indexed to true for nco:hasEmailAddress
  * Set tracker:indexed to true for nco:hasPostalAddress
  * Updated documentation for nmo, explain conversations and email
  * Added diagram to explain email classes
  * Added upstream link for nmo (semanticdesktop.org)

  Functional Tests:
  * Added new test data
  * Added test cases to verify setting localPhoneNumber
  * Added test cases for boot up scenarios
  * Updated writeback test

  libtracker-client:
  * Create vapi files with $version in the name, was static 0.7
  * Fixed vapi bindings, make Tracker.Client a Glib.Object subclass

  libtracker-miner:
  * Create vapi files with $version in the name, was static 0.7
  * Don't install tracker-miner-dbus.h, it is internal
  * Prefixed internal APIs with _ to avoid exporting unwanted symbols
  * Make sure "progress" property is set to 0.0 initially
  * Make sure we have progress >= 0.0 when crawling, using 1% for now
  * Disable monitor test case, requires unavailable internal symbols
  * Fixed memory leak when loading GKeyFile TrackerPasswordProviders

  libtracker-db:
  * Removed GValue abstraction for custom SQLite functions

  libtracker-data:
  * Added test cases for update error handling
  * Added test cases for ASK queries
  * Added test cases for 2nd 3rd and 4th predicate variables
  * Added test cases for subquery unions
  * Added test cases for restoring after journal replay
  * Refactored code for simpler subgraph contexts
  * Fixed literal binding in subqueries
  * Fixed variable binding with subqueries

  libtracker-fts:
  * Backport changes from FTS3 upstream. This fixes filtering with both, ID and MATCH constraints, which can happen when joining with unions

  tracker-store:
  * Added status notifications during journal replay
  * Removed mingw-compat.h, completely unused

  tracker-extract:
  * Don't use urn:author for nco:Contact as creator for JPEGs
  * Don't use urn:artist for nco:Contact, use urn:contact
  * Fixed OGG contact SPARQL generation, use preupdate
  * Fixed PNG contact SPARQL generation, use preupdate
  * Fixed TIF contact SPARQL generation, use preupdate
  * Cleaned up PNG, TIFF, JPEG and XMP source

  tracker-miner-fs:
  * Do not check tracker-store for ignored directories
  * Fixed potential crash when cancelling extraction

  tracker-miner-rss:
  * Removed unrequired g_free()

  tracker-control:
  * Improve output for --start so it is clearer

  tracker-status:
  * Added tracker-store journal replay status
  * Improve output so it is simpler

  tracker-search:
  * Sort results for documents, images, videos, files and folders

Bugs:

  * GB#613819, Indexing Limitations section is confusing
  * GB#613977, many text mime types are not fully extracted
  * GB#614449, tracker-extract module directory should be environment configurable
  * NB#162585, tracker-extract is crashing for mp3s with id3v24 tags
  * NB#161457, nco:creator for JPEGs has multiple dummy contacts shown in CM
  * NB#162394, We need signals for nco:IMAddress
  * NB#162028, writeback not done for nie:contentCreated
  * GB#613792, tracker-miner-fs critical log messages for jpeg extraction

Translations:

  * Updated lt: Aurimas Černius, Gintautas Miliauskas
  * Updated de: Mario Blättermann
  * Updated sl: Andrej Žnidaršič
  * Updated pl: Piotr Drąg
  * Updated es: Jorge González
  * Updated cs: Marek Černocký

Notes:

  The database version up has been incremented, this will force a
  reindex for any existing Tracker installation.


NEW in 0.7.28 - 2010-03-25
==========================

Improvements / New:

  General:
  * Fixed po/POTFILES.in, tracker-miner-rss.desktop.in was missing
  * Fixed a number of reference documentation breakages
  * Fixed some build warnings when running configure
  * Don't require Cairo and GDK for libpoppler-glib directly (they are deps)

  Ontology:
  * Added tracker:notify for mfo:Enclosure
  * Added maemo:localPhoneNumber property
  * Added nmo:MMSMessage class
  * Removed nie:InformationElement as sub-class for nmo:Message (already the case with nfo:TextDocument)
  * Removed nrl:maxCardinality for nco:url
  * Removed nrl:maxCardinality for nco:websiteUrl
  * Removed nrl:maxCardinality for nco:blogUrl
  * Removed nrl:maxCardinality for nco:foafUrl
  * Removed mto:state-user-paused
  * Removed mto:state-user-cancelled
  * Removed mto:state-user-started

  Functional Tests:
  * Writeback test cases updated for changes in sources

  ontology-validator:
  * Check for Namespace and Ontology re-definition

  libtracker-common:
  * Added tests for tracker-dbus module
  * Added tests for tracker-type-utils module (to complete tests)

  libtracker-data:
  * Added tests for journal replay
  * Added tests for ontology change coping
  * Improve journal replay time to 58s for ~53k resources (twice 0.7.26 speed)
  * Fixed warnings and errors found by vala master
  * Support journal replay of modest ontology changes

  libtracker-extract:
  * Fixed xmp tests to run only when EXEMPI is available

  libtracker-miner:
  * Fixed tests for more recent ld versions
  * Fixed includes so we only use <libtracker-miner/tracker-miner.h>
  * Added support for GIO and GIO's MOVE event.
  * Added tests for tracker-monitor module

  libtracker-fts:
  * Disable old test utils/tracker-fts/tracker-fts-test.c
  * Separate FTS index from virtual table initialization

  tracker-extract:
  * Fixed TRCK handling in MP3 extraction, don't assume they are valid
  * Removed dummy.c example, it was out of date (use examples/libtracker-extract instead)

  tracker-miner-fs:
  * Added icon for files/applications miner desktop files

  tracker-control:
  * Added pass optional all/store/miners arguments for --kill and --terminate
  * Handle binaries starting with "lt-" for testing inside src dir

  tracker-search-bar:
  * Show application's icon next to listed applications
  * Hide the results window when clicking outside it
  * Hide the results window when search entry is empty
  * Added icons for bookmark and tag results
  * Do not trigger the window popup on every key press
  * Make pixbuf size 24² to make all rows in the results the same height

  tracker-search-tool:
  * Fixed background gradient for metadata to use BG color
  * Fixed Ctrl+S so it now focuses on the search entry widget
  * Fixed metadata link buttons to be insensitive with no selection
  * Fixed metadata link buttons to ellipsize
  * Use GAppInfo to launch default applications
  * Set search entry's label mnemonic to focus search entry

Bugs:

  * GB#613562, tracker-search displays results for unavailable data

Translations:

  * Updated de: Mario Blättermann
  * Updated sl: Andrej Žnidaršič
  * Updated es: Jorge González

Notes:

  This release is considered a Release Candidate for 0.8. Assuming no
  major regressions are found with it, the next release is likely to
  be 0.8.0.

  The database version up has been incremented, this will force a
  reindex for any existing Tracker installation.

  The Deskbar applet has now been removed from Tracker.


NEW in 0.7.27 - 2010-03-19
==========================

Improvements / New:

  libtracker-common:
  * Added tests for utils, tracker_seconds_to_string(), etc.

  libtracker-extract:
  * Added tests for utils, tracker_text_normalize(), etc.
  * Added tests for xmp

  libtracker-miner:
  * Fixed miner-web introspection, "association" was enum, now bool
  * Renamed "association" to "associated"

Bugs:

  * GB#613268, missing file in tarball

Translations:

  None

Notes:

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.26 - 2010-03-18
==========================

Improvements / New:

  General:
  * Ignore -glue.h D-Bus generated files for coverage-report
  * Ignore tracker-marshal.* generated files in coverage-report
  * Added new functional tests for metadata extraction of images

  Ontology:
  * Updated nco:hasIMAddress to be a sub-property of nco:hasContactMedium
  * Updated nfo:codec range to be xsd:string, was rdfs:Literal
  * Added tracker:notify for nmm:Photo
  * Removed nrl:InverseFunctionalProperty from nco:imID property

  libtracker-common:
  * Removed unused file utility functions
  * Removed unused string utility functions
  * Removed unused TurtleWriter

  libtracker-db:
  * Don't initialize ontologies here, do it in libtracker-data
  * Removed unused D-Bus functions
  * Removed unused user-defined SQLite functions
  * Removed unused user-defined aggregate SQLite functions
  * Removed unused support for blobs

  libtracker-data:
  * Improve journal replay time from 1050s to 58s for 25249 resources
  * Move internal ontology API from libtracker-common to libtracker-data
  * Added tests for backup/restore
  * Added tests for anonymous blank nodes
  * Added tests for predicate variables
  * Do not check for ontology updates in test schema
  * Do not register properties with GObject for TrackerProperty
  * Make sure we journal super-property changes
  * Keep VmRSS under control during journal replay
  * Journal imported TTL files
  * Removed the last part of tracker:uri
  * Removed unused volume handling functions
  * Removed unused "filtered" property from TrackerProperty
  * Removed unused set_super_ functions
  * Removed unused TrackerProperty functions

  libtracker-fts:
  * Removed unused functions

  libtracker-miner:
  * Added tracker-miner-web API thanks to the work from Adrien Bustany
  * Added tests for thumbnailer API
  * Added tests for tracker-miner-manager API
  * Added tests for mock miner desktop files
  * Added TRACKER_MINERS_DIR env variable to support test cases,
    points to XML files installed by miners
  * Fixed single quote instances for SPARQL queries
  * Fixed tests, don't include specific files, link against lib instead
  * Fixed "Could not process" warnings, use g_message() instead

  libtracker-client:
  * Install Vala bindings

  tracker-extract:
  * Cleaned up oasis (OpenOffice) extractor
  * Fixes totem AUDIO_BITRATE to use nfo:averageBitrate not nmm:averageBitrate

  tracker-miner-fs:
  * Improve performance on MOVE operations, now 1s, was 9m for linux kernel
  * Don't recurse dirs from inotify if dir matches single index config

  tracker-miner-rss:
  * Added new RSS feed miner thanks to the work from Michele Tameni & Roberto Guido

Bugs:

  * GB#609607, Make tracker build without assuming implicit DSO linking
  * GB#612749, Fix include of libtracker-client/tracker-client.h in libtracker-miner/tracker-miner.h
  * GB#612873, tracker cannot extract oasis files if the filename has spaces in it.
  * GB#612744, cannot activate evolution plugin
  * GB#494441, Lack of icon for "Indexing Preferences" in Launch menu on Solaris
  * GB#494438, tracker-preferences title should be "Indexing Preferences"
  * GB#497670, Remove "startup delay" hack from the prefs
  * GB#579062, mktime() related error
  * GB#612966, Non-Recursively Indexed Directories behavior
  * GB#606443, Help kill libegg
  * GB#613114, Date string is leaked in MSOffice extractor

Translations:

  * Updated sv: Daniel Nylander
  * Updated sl: Andrej Žnidaršič
  * Updated es: Jorge González
  * Updated lt: Gintautas Miliauskas

Notes:

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.25 - 2010-03-11
==========================

Improvements / New:

  SPARQL:
  * Support WITH <uri> syntax for updates (1.1 draft), DELETE FROM /
    INSERT INTO syntax are still supported.
  * Fixed a crash on DELETEs when using blank nodes

  Ontology:
  * Updated mlo:predefined-landmark-category-accommodation nie:title
    to "Accommodation" from "accommodation".
  * Added nmo:reportDelivery.

  libtracker-common:
  * Fixed documentation breakages

  libtracker-fts:
  * Fixed prefix searches to ignore stop words
  * Fixed OR operator searching
  * Added better protection against invalid UTF-8

  libtracker-client:
  * Added TrackerSparqlBuilder documentation
  * Added tracker-client.h for all header includes (except vala built files)
  * Added vala bindings
  * Relicensed from GPL to LGPL.

  libtracker-data:
  * Revert class counts on rollback
  * Small refactor around tracker_data_update_sparql_blank()
  * Store graphs as int ID instead of g_strdup()
  * Don't g_strdup() strings which are never freed in TrackerProperty

  libtracker-extract:
  * Added example code in examples/libtracker-extract
  * Added tracker-extract.h for all header includes
  * Relicensed from GPL to LGPL.
  * Added documentation for utils (tracker_{coalesce|merge}, etc).

  libtracker-miner:
  * Fixed documentation breakages
  * Fixed move operations, index source recursively if not indexed previously
  * Added initial unit tests
  * Removed unused libraries in LIBADD

  tracker-store:
  * Fixed SubjectsAdded signal

  tracker-miner-fs:
  * Added compiler hints about unlikely code paths in common functions
  * Added --add-file option to request a file to be indexed
  * Removed all .* file/dir ignores, this is done by default
  * Removed unused libraries in LIBADD

  tracker-extract:
  * Fixed linking, only link against libtracker-common if used directly
  * Fixed playlist extractor, don't relate entries using nie:isStoredAs
  * Added --force-module for 3rd party developers to test extractor modules
  * Removed unused #define statements in all extractors
  * Removed lots of #includes which were unnecessary
  * Removed imagemagick extract, it was not even in Makefile.am

  tracker-sparql:
  * Added --notifies to list classes which notify changes

Bugs:

  * GB#612171, Add support for excel extractor to extract the plain text from the excel files
  * GB#609850, tracker-store segfault in tracker_parser_process_word
  * GB#611919, Tracker-extract-mp3 failed to extract artist name of an id3v2.2 mp3 (generated by itunes)
  * NB#156571, Aspect ratio are not extracted for video files.
  * NB#159419, Single character + * isn't working as expected for fts:match()
  * NB#158450, Tracker doesn't remove song from database when deleting song from device
  * NB#159333, wrong stats for nmo:CommunicationChannel

Translations:

  * Updated et: Mattias Põldaru
  * Updated sl: Andrej Žnidaršič
  * Updated de: Christian Kirbach, Mario Blättermann
  * Updated es: Jorge González
  * Updated cz: Marek Černocký

Notes:

  The default tracker-miner-fs.cfg ignore files and directories have
  been changed to avoid duplicate checks which adds to the indexing
  time. If you remove this file, it will be re-generated for you.

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.24
=============

Improvements / New:

  Ontology:
  * Added nao:isRelated
  * Added tracker:notify true to nmm:Playlist
  * Added tracker:isAnnotation comments
  * Show full text index flag in the documentation
  * Added table with all fts-indexed properties in documentation

  SPARQL:
  * Implicitly convert REGEX text to string, previously required str()
  * Add error reporting about nested anonymous blank nodes (see GB#611248)

  ontology-validator:
  * Re-enable compilation
  * Fixed to handle tracker:Ontology correctly

  libtracker-common:
  * Log binary name and version at beginning of log file
  * Avoid invalid memory accesses in tracker_path_list_filter_duplicates()

  libtracker-db:
  * Check disk space when recreating DB due to corruption
  * Added a check for string validity in fn:string-join

  libtracker-data:
  * Limit update buffer size, was causing high memory usage due to update buffer
  * Do not signal or journal duplicate statements
  * Avoid unnecessary RDF type URI copies on updates
  * Avoid double free on error condition
  * Propagate DB initialization error in data_manager_init

  tracker-store:
  * Remove unused statistics signal
  * Fix invalid D-Bus messages in writeback support

  libtracker-miner:
  * Added support for optical media in tracker-storage
  * Updated ::mount-point-added to include removable and optical booleans

  tracker-miner-fs:
  * Report the correct files/dirs number on miner finished
  * Improve efficiency of tracker:available updates
  * Check both directory and _contents_ rules before adding a monitor
  * Make sure we respond to IgnoreDirectories[WithContent] config changes
  * Added --eligible command line option to
  * Added support for optical media in tracker-storage

  tracker-preferences:
  * Added IndexOpticalDiscs option

  tracker-writeback:
  * Fixed compilation error in playlist writeback

  tracker-sparql:
  * Added --search option to find classes/properties

  nautilus:
  * Fixed the tags widget for the Nautilus integration

Bugs:

  * GB#611271, Preferences throttle only goes to 15/2
  * GB#611455, PPT extractor is not working is not extracting plainText
  * GB#611621, tracker-search should output filenames
  * GB#610812, Tagging a none indexed file fails silently
  * GB#611556, Need to support CD/DVD index
  * GB#611152, Adding support for msoffice2007 extraction to extract the data from pptx, xlsx, docx files
  * NB#158626, Add support for nmm:composer to MP3 extractor
  * NB#158962, tracker-miner-fs emits criticals for some desktop files
  * NB#155488, After adding a directory under "IgnoredDirectories" key...
  * NB#158864, tracker-store crashes when the file system is full
  * NB#158450, Tracker doesn't remove song from database when deleting song from device

Translations:

  * Updated pl: Piotr Drąg

Notes:

  There is a new config option called IndexOpticalDiscs in:

    $HOME/.config/tracker/tracker-miner-fs.cfg

  You will need to update your .cfg file manually OR remove the old
  file to have a new config file generated for you. The default here
  is FALSE and it covers the indexing of CDs, DVDs, etc.

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.23
=============

Improvements / New:

  libtracker-miner:
  * Fixed crash when starting tracker with CD/DVDs mounted

  tracker-extract:
  * Fixed mplayer extractor compilation
  * Fixed gstreamer g_critical() while we wait for NB#158636 fix
  * Fixed gstreamer GDate free of uninitialised pointer
  * Fixed gstreamer GDate validity checks

Bugs:

  * NB#156562, execute mode not available for executables after installation

Translations:

  * Updated et: Mattias Põldaru

Notes:

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.22
=============

Improvements / New:

  General:
  * Removed GConf dependency, no longer used
  * Added --enable-maemo configure option to discriminate 91-maemo.ontology
  * Don't link against HAL for DeviceKit-power
  * Disable flac extractor by default (has SPARQL errors, needs updating)
  * Disable libunac by default (causes tracker-store segfaults)

  Ontology:
  * Support ontology additions by 3rd party applications
  * Added tracker:Ontology class and nao:lastModified property
  * Added nco:imAvatar property
  * Added nco:default-contact-emergency
  * Added nfo:tableOfContents
  * Added tracker:fulltextNoLimit
  * Removed nrl:maxCardinality from nmm:director and nmm:leadActor

  SPARQL:
  * Fixed variable separation between multiple updates in a query

  Tests:
  * Added functional tests for named graph support
  * Added functional tests for tracker-writeback
  * Added functional tests for virtual files
  * Added libtracker-fts test cases
  * Added libtracker-miner test cases

  libtracker-common:
  * Renamed tracker-ontology module to tracker-ontologies
  * Created tracker-ontology class
  * Fixed error reporting for TrackerDateTime class
  * Don't install private header files for internal library

  libtracker-data:
  * Added ontology validation checking
  * Fixed deletion of xsd:dateTime values

  libtracker-miner:
  * Moved tracker-storage here from libtracker-common
  * Refactored tracker-storage to use GIO not HAL
  * Don't reverse lists from hash tables in tracker-storage

  libtracker-client:
  * Fixed compilation with c++ compilers

  libtracker-fts:
  * Limit word length based on ontology (was > 3)

  tracker-store:
  * Reduced amount of memory used for events

  tracker-miner-fs:
  * Ignore VIM files by default
  * Increment processed error count on SparQL errors
  * Ensure the full directory hierarchy is inspected on reindex
  * Improved logging for mount points added or removed

  tracker-extract:
  * Fixed handling of album gain properties in gstreamer module

  tracker-search-bar:
  * Fixed headers for panel applet
  * Used correct panel_applet_ functions not panel_applet_gconf_*

Bugs:

  * GB#610609, libtracker-client: Don't include a file between G_{BEGIN,END}_DECLS
  * GB#609075, Adding support for pdf extractor to extract the index data from the pdf files
  * GB#609004, Timeout on 1 file during extraction causes 10 to fail
  * GB#610882, Segfault with sparql queries using undefined variables
  * NB#157813, tracker writeback is not updating nmm:camera details correctly
  * NB#157429, tracker writeback is not updating values of all supported properties.
  * NB#157814, nmm:flash value is not changing after tracker-writeback , value is always set to nmm:flash-off
  * NB#157822, mlo:location , mlo:country , mlo:city , mlo:state , mlo:address values are not updated after changing the values with tracker-writeback
  * NB#157286, tracker-search is case sensitive for stopwords
  * NB#157899, incorrect value set for dates without TZD part or TZD with minutes

Translations:

  * Updated sl: Andrej Žnidaršič
  * Updated nb: Kjartan Maraas

Notes:

  The database version up has been incremented, this will force a
  reindex for any existing Tracker installation.

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.21
=============

Improvements / New:

  General:
  * Bumped GTK+ requirement to 2.18

  Tests:
  * Restructured data and added metadata extraction test

  SPARQL:
  * Support simple named graph patterns in queries
  * Added fn:*-from-dateTime functions.

  libtracker-db:
  * Fixed compiler warnings in function_sparql_cartesian_distance()
  * Updated journal to support named graphs

  libtracker-data:
  * Support retrieving localtime for stored date/time values, was UTC

  libtracker-common:
  * Fixed header filename for TurtleWriter
  * Added TrackerDateTime fundamental GType for time + UTC offset

  libtracker-client:
  * Refactored away tracker-statement-list API

  libtracker-miner:
  * Refactored TrackerCrawler so we process parent dirs before children

  libtracker-extract:
  * Moved libtracker-common functions here only used in tracker-extract
  * Fixed documentation example since preupdate was added

  tracker-extract:
  * Fixed linking so we include libtracker-extract for extractors
  * Added missing warning if required GModule functions are not found
  * Use artist's value in filename when creating new album art

  tracker-info:
  * Print queried URN (since we use anonymous nodes for files now)

  tracker-tag:
  * Fixed looking up tags by filename and don't require explicit paths
  * Fixed looking up tags, don't use regular expressions, use exact matching
  * Removed unused code and fixed memory leak when removing tags

  tracker-search-tool:
  * Added gradient fill to metadata tile
  * Fixed display of type and file size in metadata tile

  nautilus:
  * Don't show an empty file count when adding new tags

  kmail:
  * Fixed linking so we include libtracker-client

Bugs:

  * GB#610026, Nautilus Tagging Extension does not work at all.
  * GB#610318, Bump GTK+ dep to 2.18 due to gtk_widget_set_can_default()
  * GB#610360, Tracker adds multiple entries for files
  * NB#157277, Not able to search for files based on a particular tag
  * NB#149503, ClassUpdateSignaler for MusicPiece doesn't signal subjectsChanged signals

Translations:

  * Updated de: Mario Blättermann
  * Updated cs: Petr Kovar
  * Updated es: Jorge González

Notes:

  The database version up has been incremented, this will force a
  reindex for any existing Tracker installation.

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.20
=============

Improvements / New:

  General:
  * Updated ttl_sgml documentation tool to add notice for deprecations

  Ontology:
  * Fixed range of dc:source
  * Changed domain of nmo:isAnswered from nmo:Email to nmo:Message
  * Add IMAddress and adapt existing IM classes and properties for NCO
  * Deprecated nmm:length and replaced all uses with nfo:duration

  Tests:
  * Added more functional tests for distance ontology

  libtracker-common:
  * Don't emit multiple changed signals for one edit in TrackerConfigFile
  * Added tracker_gslist_with_string_data_equal()
  * Fixed documentation warnings/missing chapters

  libtracker-client:
  * Moved tracker-sparql-builder here from libtracker-common
  * Port tracker_sparql_builder_object_unvalidated() to Vala
  * Fixed documentation warnings/missing chapters
  * Fixed linking using -export-symbols-regex
  * Fixed pkg-config .pc file

  libtracker-data:
  * Fix error handling when using non-existing class

  libtracker-miner:
  * Use G_MAXINT for tracker D-Bus timeout
  * Refine the list of exported symbols
  * Make tracker-{miner-manager|crawler}.h part of the public API
  * Renamed some APIs in TrackerMinerFS
  * Don't print estimated time remaining message more than once/5 seconds
  * Renamed the Writeback() D-Bus method to IgnoreNextUpdate()
  * Fixed G_IS_OBJECT(file) crash
  * Fixed check for processing state in crawl_directories_start()
  * Fixed documentation warnings/missing chapters
  * Fixed linking using -export-symbols-regex
  * Fixed pkg-config .pc file

  libtracker-extract:
  * Do not include tracker-statement-list.h
  * Fixed documentation warnings/missing chapters
  * Fixed linking using -export-symbols-regex
  * Fixed pkg-config .pc file

  tracker-miner-fs:
  * Fixed nfo:belongsToContainer.
  * Add support for real time updates of the indexed directories from config

  tracker-writeback:
  * Use G_MAXINT for tracker D-Bus timeout

  tracker-extract:
  * Make sure we generate empty SPARQL statements
  * Fixed freeing a constant
  * Fixed memory problems with MSOffice extractor
  * Support for nmm:lyricist in MP3 files

  tracker-search-tool:
  * Fixed to work with recent anonymous file node changes

  tracker-search-bar:
  * Use the tracker icon for the tracker-search-bar applet

Bugs:

  * GB#609532, Expand search for album coverart
  * GB#609327, Remove libtracker-gtk
  * GB#609607, Make tracker build without assuming implicit DSO linking
  * NB#155064, Height and width are not extracted for images of GIF formats
  * NB#156571, Aspect ratio are not extracted for video files

Translations:

  * Updated sv: Daniel Nylander
  * Updated es: Jorge González
  * Updated nb: Kjartan Maraas

Notes:

  The database version up has been incremented, this will force a
  reindex for any existing Tracker installation.

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.19
=============

Improvements / New:

  libtracker-common:
  * Normalize paths so getting the UDI for the mount dir GFile works.
  * Fixed memory leak in tracker_storage_get_volume_udi_for_file()

  libinotify:
  * Fixed move directory monitor failure from reused watch descriptors.

  libtracker-miner:
  * Get all children URIs in a single query when moving a folder.

  libtracker-extract:
  * Updated extractors to return an embedded insert together with
    preupdate. Final insert is composed in tracker-miner-fs with file
    data too.

  tracker-miner-fs:
  * Fixed to work with anonymous file nodes.
  * Use nie:isStoredAs for the file's URN.

  tracker-extract:
  * Fixed to work with anonymous file nodes.
  * Fixed page leaks in PDF extractor.
  * Fixed 5 memory leaks in PDF extractor.
  * Fixed compilation error with the vorbis extractor.
  * Added FLAC extractor.
  * Always favor XMP over IPTC, EXIF or native formats for images.

  tracker-search-bar:
  * Fixed to work with anonymous file nodes.

  tracker-info:
  * Fixed to work with anonymous file nodes.

  tracker-search:
  * Fixed to work with anonymous file nodes.

  tracker-tag:
  * Fixed to work with anonymous file nodes.

  nautilus:
  * Fixed to work with anonymous file nodes.

  examples:
  * Removed "translatable" property from UI files.

Bugs:

  * GB#608395, Flac extractor
  * GB#607576, Nautilus extension: A tag might be a nie:InformationElement too
  * GB#607699, Tracker fails to compile with evolution 2.29.x
  * NB#155435, tracker is not extracting all the metadata details for MP4 Video files
  * NB#152635, nfo:isContentEncrypted is not populated for DRM protected files

Translations:

  * Updated zh_CN: Aron Xu

Notes:

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.18
=============

Improvements / New:

  General:
  * Added configure option for enca.
  * Fixed bashism in configure.
  * Fixed compilation warnings.

  Ontology:
  * Include diagrams and explanations in generated documentation.
  * Reformatted documentation to docbook.

  SPARQL:
  * Add tracker:cartesian-distance and tracker:haversine-distance
    functions to calculate distance between two geolocations.
  * Support constructor functions for type conversion.

  libtracker-db:
  * Create directory structure before creating journal file.

  tracker-miner-fs:
  * Support reindexing by mime type for new extractors.
  * Improve logging of files processed/notified.

  tracker-writeback:
  * Fixed manual white-balance writeback.

  tracker-search:
  * Fix album listing.

  evolution:
  * Timed waiting on the GCond.

Bugs:

  * NB#153866, Removing and selecting a resource right afterwards...

Translations:

  * Updated es: Jorge González
  * Updated et: Mattias Põldaru
  * Updated hu: Gabor Kelemen
  * Updated sl: Andrej Žnidaršič

Notes:

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.17
=============

Improvements / New:

  General:
  * Build now uses separate m4/ directory.
  * Build now uses AC_CONFIG_HEADERS.
  * Removed all thumbnail related code. It is no longer used.

  Ontology:
  * Added mlo:GeoLocation, mlo:GeoSphere, mlo:GeoBoundingBox classes.
  * Added mlo:LocationBoundingBox, mlo:Route, classes.
  * Added mlo:LandmarkCategory, mlo:isRemovable classes.
  * Added mlo:belongsToCategory property.
  * Added mlo:bbNorthWest, mlo:bbSouthEast properties.
  * Added mlo:radius, mlo:asBoundingBox, mlo:asGeoPoint properties.
  * Added mlo:asPostalAddress property.
  * Added mlo:predefined-landmark-category-accommodation.
  * Added mlo:predefined-landmark-category-business.
  * Added mlo:predefined-landmark-category-communication.
  * Added mlo:predefined-landmark-category-educational-institute.
  * Added mlo:predefined-landmark-category-entertainment.
  * Added mlo:predefined-landmark-category-food-beverage.
  * Added mlo:predefined-landmark-category-geographical-area.
  * Added mlo:predefined-landmark-category-outdoor-activities.
  * Added mlo:predefined-landmark-category-people.
  * Added mlo:predefined-landmark-category-public-service.
  * Added mlo:predefined-landmark-category-religious-places.
  * Added mlo:predefined-landmark-category-shopping.
  * Added mlo:predefined-landmark-category-sightseeing.
  * Added mlo:predefined-landmark-category-sports.
  * Added mlo:predefined-landmark-category-transport.
  * Changed mlo:altitude to be a double not int.
  * Deprecated mlo:boxSouthWestCorner, mlo:boxEastLimit.
  * Deprecated mlo:boxVerticalLimit, mlo:boxNorthLimit.
  * Deprecated mlo:city, mlo:country mlo:state, mlo:address.
  * Deprecated mlo:poiLocation.

  data-generators:
  * Add channel/conversation to IM test data.
  * Use nco ontology updates.
  * Use nmo:lastMessageDate, nmo:communicationChannel,
    nco:default-contact-me.

  libtracker-db:
  * Fixed crash in journal if reader_init() failed.
  * Fixed error propagation in vapi files.
  * Fixed make distcheck failure for journal test case.

  litracker-data:
  * Fixed error propagation in vapi files.
  * Fixed build issues with missing SQLite CFLAGS.

  tracker-store:
  * Fixed a potential crash returning NULL for sparql_update_blank().

  libtracker-extract:
  * Created this new library for 3rd parties to build extract plugins.

  libtracker-miner:
  * Use the version in the built .so filename.
  * Don't use resource iri = uri.
  * Stopped using tracker:uri.

  tracker-miner-fs:
  * Added hidden directory checks for FAT file systems.

  tracker-extract:
  * Cleaned up IPTC code.
  * Added parameter checks for exif/xmp/iptc public APIs.
  * Added full text extraction support from MSWord files for FTS.

  tracker-search:
  * Warn about using stop words for search terms.

  evolution:
  * Fixed a UI hang.

  nautilus:
  * Fixed build issues with missing D-Bus CFLAGS.

  kmail:
  * Fixed dist tarball by including the missing registrar.xml file.

Bugs:

  * NB#144725, Tracker doesn't add ontologies/tracker#added prop...
  * NB#149100, nmm::MusicAlbum does not update when music album...
  * NB#153444, tracker-stats giving incorrect results for contacts...
  * NB#153335, tracker-search -a not returning actual album count

Notes:

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.16
=============

Improvements / New:

  Ontology:
  * Added nmo:isSent property.
  * Added mlo:Landmark and mlo:PointOfInterest classes.
  * Added mlo:poiLocation property.
  * Added nao:predefined-tag-favorite.
  * Added nco:im-capability-text-chat.
  * Updated the domain of nmo:isDraft from nmo:Email to nmo:Message.
  * Updated comments for mlo:altitude, rdfs:label and rdfs:comment.

  libtracker-common:
  * Added D-Bus client lookup for logging.
  * Added tracker_{class,property}_{get,set}_id() APIs.
  * Fixed some uint vs int issues in TrackerClass.
  * Fixed all resource IDs to be gints.

  libtracker-db:
  * Added binary log to journal SPARQL transactions.
  * Added database checks for corruption.
  * Fixed all resource IDs to be gints.

  litracker-data:
  * Never delete URI <-> ID mapping.
  * Reuse resource_cache and resources in update buffer.
  * Reuse blank_buffer arrays.
  * Avoid querying for ID twice when inserting resources.
  * Avoid string manipulation in get_property_values.
  * Fixed all resource IDs to be gints.

  libtracker-client:
  * Updated TrackerClient to be a GObject.
  * Updated documentation with deprecation information.
  * Added tracker_client_new() to replace tracker_connect().
  * Use GSlice for common allocations.
  * Seal public members in private struct.
  * Make D-Bus interface/service/object definitions public.
  * Don't start D-Bus service just check Tracker is activatable.

  libtracker-miner:
  * Fixed missing D-Bus release call when cleaning up miner.

  tracker-store:
  * Added D-Bus sync method for the journal.
  * Removed non-existant functions in tracker-dbus.h

  tracker-search:
  * Improved -a output, don't prepend "album" on each line.

  tracker-extract:
  * Set nmm:musicAlbum and nmm:performer in GStreamer plugin.

  evolution:
  * Throttle 100 emails per 5 seconds.
  * Fixed a crash in evolution plugin when killing tracker-store.
  * Fixed reference leak on TrackerMiner.
  * Fixed exit() call when enabling/disabling plugin.

Bugs:

  * NB#151295, Tracker goes to error state with certain sparql updates

Notes:

  The fix for the exit() in Evolution when enabling/disabling the
  plugin is not completely fixed. If you start with the plugin
  disabled and enable it, it still happens (but not the other way
  round). This part needs fixing in Evolution. See bug:

    https://bugzilla.gnome.org/show_bug.cgi?id=606940

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.15
=============

Improvements / New:

  General:
  * Fixed id3lib configure errors.
  * Improved README for people building Tracker.

  SPARQL:
  * Fixed non-ASCII IRIs.
  * Fixed inserting negative integers without needing quotes.

  libtracker-fts:
  * Simplify virtual table creation.

  libtracker-data:
  * Separate inserting URIs from creating DB schema.

  tracker-extract:
  * Added nmm:length for audio files from GStreamer extractor.
  * Fixed nfo:channels and nfo:sampleRate, uint vs int error.

Bugs:

  * Fixes GB#572237, Remove deprecated Glib symbols
  * Fixes GB#604655, Fix detection of ioprio
  * Fixes NB#132761, Tracker does not extract metadata from WMA files

Notes:

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.14
=============

Improvements / New:

  libtracker-common:
  * Fixed regression in URI escaping.

  tracker-extract:
  * Improved IPTC extractor.
  * Fixed memory leak in Vorbis extractor.
  * Fixed xine extractor.

  tracker-writeback:
  * Added support for CountryName in XMP writeback.

Bugs:

  None.

Notes:

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.13
=============

Improvements / New:

  General:
  * GLib requirement is now 2.20 from 2.16.
  * Removed all imported GLib code borrowed from >= 2.16.
  * Merged coalesce branch to master.
  * Merged writeback branch to master.

  Ontology:
  * Added tracker:coalesce (coalesce branch merge).
  * Added mlo:city, mlo:country, mlo:state and mlo:address.
  * Added cardinality for nfo:entryCounter and nfo:listDuration.

  SPARQL:
  * Added tests for functions.

  libtracker-common:
  * Small improvements to TrackerLanguage documentation.

  tracker-miner-fs:
  * Fixed nie:url property to be a string, not a resource.

  tracker-extract:
  * Fixed duplicate field usage in jpeg, pdf, tiff, png and xmp.
  * Fixed to use new mlo ontology in jpeg, pdf, tiff, png and xmp.
  * Fixed xmp left-top orientation.

  tracker-writeback:
  * Disabled MP3 module for now, reports of MP3 corruption.

  nautilus:
  * Fixed crash when showing the context menu for the background.

  evolution:
  * Fixed nie:url property to be a string, not a resource.

  kmail:
  * Fixed nie:url property to be a string, not a resource.

Bugs:

  None.

Notes:

  The database version up has been incremented, this will force a
  reindex for any existing Tracker installation.

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.12
=============

Improvements / New:

  General:
  * Removed emacs style editing style at the top of all .[ch] files.
  * Removed extensions/ (was thunderbird/firefox legacy code).
  * Clean up python/ (moved useful code, removed the rest):
    * Moved gtk-sparql to utils/
    * Moved deskbar-handler to src/plugins/deskbar/
  * Added coverage analysis using lcov.
  * Merged nautilus-extension branch to master (tagging support).

  Ontology:
  * Automatically generate ontology docs using gtk-doc.
  * Added nmo:Conversation class and nmo:conversation property.
  * Added nmo:lastMessageDate and nmo:hasParticipant properties.
  * Add nmo indexes for important message properties.
  * Deprecated nmo:plainTextMessageContent (for nie:plainTextContent)
  * Documented nmo more thoroughly.

  SPARQL:
  * Support functions with non-prefixed IRI.
  * Do not support AS outside of select expressions.

Bugs:

  * Fixes: GB#603054, fails to build on missing tracker-explorer
  * Fixes: NB#150158, tracker-store memory usage grows a lot dur...

Notes:

  The database version up has been incremented, this will force a
  reindex for any existing Tracker installation.

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.11
=============

Improvements / New:

  General:
  * All indentation and trailing whitespace has been fixed.

  Ontology:
  * Added nao:deprecated to the ontology.
  * Added support for class signals to nmm:RadioStation.
  * Added small utils/ontology app to generate graphs.
  * Use nie:url instead of nie:isStoredAs. Discussed here:
    https://sourceforge.net/apps/trac/oscaf/ticket/46
  * Marked nie:isStoredAs as deprecated.
  * Marked nfo:fileURL as deprecated.
  * nmo:Mailbox and nmo:MailAccount inheritance order swapped.
  * mfo:FeedElement now is a subclass of nie:InformationElement.

  SPARQL:
  * Added subquery capability.
  * Translate AS ?foo to SQL AS foo.
  * Support GROUP BY from SPARQL 1.1 draft.
  * Support AS ?foo from SPARQL 1.1 draft.
  * Support bracketted expressions as select expressions.
  * Fixed type handling in aggregate functions (improves performance).

  Tests:
  * Added for libtracker-data aggregates (distinct, group, etc).

  libtracker-common:
  * Fixed tracker-utils.h to use G_{BEING|END}_DECLS, broke c++ builds.

  libtracker-data:
  * Fixed property functions to support resource variables.
  * Fix reading inverse functional properties from DB.

  libtracker-client:
  * Added more documentation.

  libtracker-miner:
  * Added more documentation (100% coverage now).

  tracker-extract:
  * Fixed gstreamer the way nmm:setNumber, nfo:genre are set.
  * Refactored Microsft Word extractor to use libwv2 for FTS.
  * Added support for Microsoft Power Point files.

  tracker-search-bar:
  * Fixed results window to hide if text is empty, show if not.

Bugs:

  GB#604152, tracker-miner-fs crashes after initial startup

Notes:

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.10
=============

Improvements / New:

  Ontology:
  * Added nco:IMCapability* and nco:imContactCapability*.
  * Added tracker:notify to nmo:Message.
  * Added maemo:webhistoryThumbnail.
  * Fixed maemo:relevance to use maxCardinality of 1, None previously.

  Documentation:
  * Fixed missing docs/ontology PNG files.

  Data Generators:
  * Fixed Python 2.5 requirement.
  * Fixed to work with the latest ontology.
  * Fixed to not generate duplicate self entries for nco:PersonContact

  libtracker-common:
  * Fixed memory leaks in TrackerProperty finalize.
  * Fixed classes/props/namespaces to use GPtrArray bookkeeping.

  libtracker-data:
  * Fixed memory leak in tracker_data_update_sparql{_blank}().

  libtracker-miner:
  * Fixed memory leaks in querying code.
  * Fixed memory leaks when removing and moving items.
  * Fixed memory leak in monitor code when freeing events.
  * Fixed some build warnings.
  * Fixed to not check for locks in non-existent files.
  * Removed TrackerMiner::terminated signal, it was unused.

  libtracker-client:
  * Added initial documentation. About 50% is documented.

  tracker-miner-fs:
  * Fixed memory leaks in mount code.

  tracker-tag:
  * Now uses g_file_new_from_command_line_arg() so URIs don't have to
    be full URIs.

  tracker-writeback:
  * Fixed missing include for locale.h

  tracker-search-bar:
  * Fixed all queries to use FTS not FILTER SPARQL.
  * Fixed to be able to launch applications and all files too.
  * Updated to show 10 items per category not 5 by default.
  * Added support for tags, bookmarks, contacts and websites.

Bugs:

  None.

Notes:

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.9
============

Improvements / New:

  General:
  * Bumped libexempi requirement from 1.99.2 to 2.1.0 for NS_PDF.
  * Fixed linking errors found with binutils-gold ld.

  Ontology:
  * New property tracker:writeback used to know if ontology should be
    written back to their original files (if supported by
    tracker-writeback).
  * Added nmm documentation.
  * Added nmm:dlnaProfile and nmm:uPnPShared properties.
  * Added mfo documentation.
  * Added mfo:optional property.
  * Added nmm:alternativeMedia property.
  * Renamed nmm:meteringMode-* to nmm:metering-mode-*.
  * Renamed nmm:whiteBalance-* to nmm:white-balance-*.
  * Updated mlo, ncal, mfo, nmm, mto, nao, nfo + nco documentation.

  SPARQL:
  * Updated to return generated URIs for inserted blank nodes.

  libtracker-common:
  * Fixed removable device code to include volumes mounted in /media

  libtracker-data:
  * Fixed build with vala master.
  * Fixed to accept NULL for GErrors.
  * Added support for returning generated uuid URIs for updates.

  libtracker-fts:
  * Fixed crash in TrackerParser's parser_next() function.
  * Fixed invalid read in TrackerParser.

  libtracker-miner:
  * Updated to use standard GIO callbacks (GAsyncResult, etc).
  * Removed redundant monitor code.
  * Fixed memory corruption for directory moves in monitor code.
  * Fixed test/ to use ::finished not ::terminated which is unused.
  * Moved test/ directory to examples/libtracker-miner.

  tracker-miner-fs:
  * Added $HOME as a single level directory to index by default.
  * Removed duplicate call to tracker_storage_new() / hal debug dump.

  tracker-preferences:
  * Fixed title, use "Tracker Preferences" not "tracker-preferences".
  * Moved "Indexing" options to their own tab.
  * Added mnemonics for notification area options.

  tracker-writeback:
  * New binary for writing metadata back to files. Initial support for
    XMP and MP3 for now.

  tracker-extract:
  * Fixed XMP coalescing for TIFF/PNG/PDF/JPEG.
  * Fixed build warning with mockup_SOURCES.

  rss_reader:
  * Fixed exception handling on import.

Bugs:

  * Fixes NB#135360, Images from all memory location are not disp...
  * Fixes GB#598470, Search entry icons grayed out
  * Fixes GB#600973, tracker-control: don't use "-h" to remove th...

Notes:

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.8
============

Improvements / New:

  General:
  * Add --version option to command line tools.

  Ontology:
  * NCO: Add more IM properties.
  * NMO: Add first draft of SMS support.
  * Work on ontology description and documentation.

  SPARQL:
  * Limit scope of blank nodes to conform to specification.
  * Fix missing results due to OPTIONAL optimization.

  libtracker-data:
  * Fix critical warning on tracker-store startup.

  tracker-extract:
  * Fix use of tagreadbin in GStreamer extractor.
  * Use NAO tags for image keywords.
  * Correctly detect 3GPP audio files in GStreamer extractor.
  * Extract nmm:albumTrackCount from MP3 files.

  RSS Reader:
  * Add some Zeitgeist interaction to the RSS Reader example.

Notes:

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.7
============

Improvements / New:

  Ontology:
  * Mark nco:phoneNumber, nco:imID, and nco:emailAddress as inverse
    functional properties.

  SPARQL:
  * Optimize simple optionals with inverse functional properties.
  * Return property URI instead of internal uri for fts:offsets().

  libtracker-data:
  * Propagate SQLite errors on updates.

  tracker-miner-fs:
  * Handle errors when moving items recursively.


NEW in 0.7.6
============

Improvements / New:

  General:
  * Fixed libvorbis detection in configure.
  * Fixed data/.gitignore to not exclude .desktop.in files.
  * Added AC_CACHE_CHECK for ioprio in configure for cross-platform
    compilation.
  * Imported data generation scripts from old repository.

  Ontology:
  * Fixed nmm:exposureTime to be a double not integer.
  * Added tracker:weight to ontology properties (was only used on few
    properties before like nie:title). Used for scoring/ranking.
  * Added more FTS fields so FTS matching occurs on more useful data.
  * Added nco:metacontact concept
    (see http://live.gnome.org/Tracker/Documentation/ContactsOntology)
  * Removed cardinality restriction for dc:contributor.
  * Removed cardinality maximum from nco:contrbutor & nco:creator.
  * Renamed nmo:VOIPCall to nmo:Call.

  SPARQL:
  * Fixed fts:rank when fts:match is not first predicate in query.

  Functional Tests:
  * Added for fts:rank
  * Added for nco:metacontact

  libtracker-common:
  * Fixed tracker_coalesce() to not use empty/blank strings.
  * Fixed tracker_is_blank_string() to be UTF-8 compliant.
  * Added tracker_sparql_builder_object_double() method.

  libtracker-data:
  * Fixed assertion using cursors open twice (tracker-store crash).
  * Fixed outer query cursors being used by inner queries.

  libtracker-miner:
  * Fixed calls to tracker-store to be completely async (no blocking
    when tracker-store is not available)
  * Fixed trying to free invalid GPtrArray pointers.
  * Updated reference documentation.
  * Improve error reporting when initializing volumes.

  tracker-store:

  tracker-extract:
  * Fixed return value in albumart_set().
  * Fixed JPEG to coalesce nie:comment.
  * Fixed JPEG leak if file size is < 18 bytes.
  * Fixed MP3 id3v20/v22 memory corruption.
  * Fixed MP3 id3v20/v22 comments retrieval.
  * Fixed cases where _object_unvalidated() was used over _object().
  * Added album art support with libquill (alternative to GdkPixbuf).
  * Updated ALL plugins to use TrackerSparqlBuilder API (from older
    variant), this produces less D-Bus traffic and the output is
    generally nicer.

  tracker-search-bar:
  * Fixed to show all documents, not just paginated ones.
  * Fixed icon background to be transparent.
  * Fixed showing results and no results label at the same time.
  * Fixed window height to be a lot longer by default.
  * Fixed selecting blank rows for separators.
  * Fixed showing blank rows after all results.
  * Fixed sorting (broken by use of hash table).
  * Added vertical orientation for applet.
  * Added ranked sorting for searches.
  * Added limit for each category to 5.
  * Added interactive searching on a 300ms delay.

  tracker-search-tool:
  * Added interactive searching.

Bugs:

  * Fixes GB#600081, Get rid of Bonobo

Notes:

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.5
============

Improvements / New:

  SPARQL:
  * Fixed to flush update buffer after DROP GRAPH.
  * Fixed leaking variables in reference cycle.
  * Support fts:rank and fts:offsets in function queries.

  libtracker-common:
  * Fixed battery handling so we don't throttle indexing when on low
    battery in cases where we are charging on AC power.
  * Fixed logging so that verbosity=1 displays all G_LOG_LEVEL_INFO
    messages. Previously it was unused.
  * Fixed vapi dependencies in Makefile.am
  * Added tracker_info() and tracker_dbus_request_info() to use
    G_LOG_LEVEL_INFO logging.

  libtracker-db:
  * Added tracker_db_statement_bind_null(). This allows use of ?
    parameters in SQL to be set to NULL. No longer special casing NULL
    when used as a value when deleting single valued property values.

  libtracker-data:
  * Fixed crash from dereferencing GError when NULL in
    delete_metadata_decomposed().
  * Added a rollback callback to prevent sending update events which
    are rolled back.

  libtracker-fts:
  * Use tracker:weight for per-property weight.

  libtracker-miner:
  * Fixed overwriting throttle in TrackerMinerFiles class
    construction.
  * Fixed TrackerCrawler class vtable definition of ::finished.
  * Fixed g_critical() if .service files are not available for some
    miners (this is the case for the Evolution plugin which is not
    started by D-Bus but Evolution).
  * Fixed tracker_connect() call to disable D-Bus timeouts.

  tracker-store:
  * Compress multiple ClassSignals for predicate-values into a single
    signal.

  tracker-extract:
  * Fixed libstreamanalyzer plugin so it doesn't report file ontology
    information which miner-fs already provides.
  * Fixed configure script so libstreamanalyzer is disabled by default
    but can be forced with --enable-libstreamanalyzer. Previously it
    was just compiled if it was available.
  * Fixed log spamming on start up, based on the fix for NB#126773 for
    0.6. See commit 65adc16dc0183a1a9b08eb85440312399748262a.
  * Fixed missing "break;" when parsing id3v20 tags leading to a
    double free with genre/copyright assignment.
  * Fixed MP3 extractor memory leaks in id3v24 comment tag and tlen
    tag.
  * Fixed so we initialize album art when running stand alone with -f
    command line switch (fixes critical warnings from non-initialized
    caches).
  * Added support for nmm:trackNumber for MP3 extractor. Also uses an
    integer not a string which was leaking too.
  * Added command line switch -i to force in-house extractors over
    libstreamanalyzer.
  * Removed use of g_build_filename() for log filename, instead use the
    string returned by tracker_log_init().

  tracker-control:
  * Created from tracker-processes (which no longer exists).
  * Added support for removing all thumbnails.
  * Added support for removing all config files.
  * Added support for starting Tracker's miners.

  tracker-status:
  * Fixed crash when GValue is NULL when getting status/progress.

  tracker-sparql:
  * Fixed use of '#' and use ':' instead. For example nfo#Video.

  tracker-explorer:
  * Improved UI layout to be more inline with the HIG.

  tracker-status-icon:
  * Added man page for the .cfg file.

  tracker-search-tool:
  * Fixed mimetype query.

Bugs:

  * Fixes NB#120255, Tracker doesn't read "year" tag from the mp3...

Notes:

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.4
============

Improvements / New:

  General:
  * Bumped database schema version (we were not doing this for 0.7.x
    until now) which makes sure a reindex occurs when the database
    schema changes. This is to prevent bugs like: GB#598224.

  Ontology:
  * Added maemo:relevance to provide additional hit scoring.

  SPARQL:
  * Updated to refactor code duplication.
  * Fixed case sensitivity for variables

  Database:
  * Removed EvolutionModseq from the Options table - unused.

  libtracker-common:
  * Fixed typo in devicekit low battery functionality, this lead to
    indexing being paused in the wrong conditions.
  * Fixed TrackerConfigFile so inotify CREATE events trigger changed
    signal.

  libtracker-data:
  * Updated to support multiple resources in queries to improve
    performance for specific inserts requests.

  libtracker-miner:
  * Fixed re-index every time tracker-miner-fs was started due to
    mtime query error since introducing timezones.

  tracker-extract:
  * Fixed so words indexed for FTS is based on tracker-fts.cfg, not
    just static at 1000.

  tracker-search-tool:
  * Fixed so only one instance can be run at a time.

  tracker-evolution-plugin:
  * Fixed so we don't spam status updates about the miner.
  * Fixed OOM situation by using D-Bus noreply functions instead.
  * Fixed memory leak when receiving errors from D-Bus responses.
  * Fixed memory leak when processing folders.
  * Fixed progress reporting to go forward not jump around.

Bugs:

  * Fixes GB#570608, Translation of firefox-extension
  * Fixes NB#135969, Artist and Album query returns reults other than...

Notes:

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.3
============

Improvements / New:

  General:
  * Require SQLite >= 3.6.16. Required for test cases to pass.
  * Removed requirement for 'sqlite_exec' in configure.ac.

  RSS Reader:
  * Updated Python example to use new Tracker APIs.

  Ontology:
  * Added nmm:albumArtist property.
  * Added nmm:Playlist class.
  * Removed cardinality restriction for nie:informationElementDate.

  SPARQL:
  * Fixed time zone handling when inserting time_t.
  * Support single-valued properties as functions.
  * Support multi-valued properties as functions.
  * Improved to not require 'AS' when using SELECT functions.
  * Support rollback for updates when one or more errors occur.

  libtracker-db:
  * Make sure SQLite statements are reset when using cursors. This was
    leading to DB locks.

  libtracker-miner:
  * Fixed infinite loop when unmounting.

  tracker-store:
  * Removed all use of tracker-storage for volume handling. This is
    now done in tracker-miner-fs.

  tracker-preferences:
  * Added visibility options for tracker-search-tool.

  tracker-status-icon:
  * Added tracker-status-icon.cfg for visibility settings.
  * Added global binding to launch tracker-search-tool (<Ctrl><Alt>S).
  * Improved so progress bars are not so tall and the now pulses when
    at 0% (used for crawling or initialising usually).
  * Improved so the progress bar is not displayed for miners not
    running.
  * Fixed so the default progress is 100% (in cases where the
    tracker-status-icon is restarted).

  tracker-search-tool:
  * Fixed UI to be more HIG compliant.

Bugs:

  * Fixes GB#590501, Album art heuristic always fails
  * Fixes NB#137046, Properties Are Not Retained Upon File Modific...
  * Fixes NB#141698, List of saved playlist has names of albums too
  * Fixes NB#134107, Optional properties with orderby causes signi...

Notes:

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.2
============

Improvements / New:

  General:
  * Added support for AM_SILENT_RULES (used for producing nice build
    output), see http://live.gnome.org/GnomeGoals/NicerBuilds
  * Removed filters/ - they are no longer used, was for full text
    search abilities. We now do this in tracker-extract.
  * Simplify CFLAGS/LIBS for applications and generally all Makefiles.

  Ontology:
  * Added average{audio|video}bitrate for nmm:Video. This allows the
    audio and video to have different bit rates in the same URN.
  * Added properties for newly added MTP.
  * Added properties fourCC and waveformat for MTP.
  * Added properties to NMM to support MTP (nmm:lyrics,
    mmm:skipCounter, nmm:isCropped, nmm:isColorCorrected).
  * Added nmm and mto prefixes.
  * Updated mtp:ScanType to subclass nie:InformationElement allowing
    nie:description on instances.
  * Renamed nmm:album to nmm:videoAlbum to be consistent with
    nmm:audioAlbum.

  Test Cases:

  libstemmer:
  * Fix API to return const/non-const more consistently.

  libtracker-common:
  * Fixed TrackerConfigFile to make it an abstract type object.
  * Fixed storage (HAL) APIs to return const/non-const more
    consistently.
  * Fixed tracker_path_evaluate_name() so we use the HOME environment
    variable with g_getenv() instead of g_get_home_dir(). This fixes
    issues seen with NFS mounted home directories.
  * Fixed storage (HAL) APIs to normalise directories returned.
  * Fixed timezone test case for tracker-type-utils.
  * Improve documentation.

  libtracker-data:
  * Fixed virtual property tracker:uri.

  libtracker-miner:
  * Added "process-pool-limit" to TrackerMinerFS, this allows us to
    process more than one file at once. Previously each file was
    extracted in tracker-extract and stored in tracker-store
    sequentially. Now data is extracted while it is being stored if
    process-pool-limit is > 1. Now it is set to 10.
  * Added support for deleting recursively when a directory is removed.
  * Updated the documentation.
  * Removed "application" parameter in tracker_miner_pause(), since
    it now uses g_get_application_name() or TrackerMiner::name
    internally.
  * Update vtable function process_file() for TrackerMinerFS into a
    signal to be consistent with the rest of the API.
  * Fixed pkg-config file to include dbus-glib.
  * Fixed moving items between two monitored directories. Was unsupported.
  * Fixed g_timer_destroy() warning with timer != NULL.

  tracker-store:
  * Updated the Dia documentation for trackerd to detail how
    tracker-store works.
  * Updated SQL generated for FTS queries by SPARQL.
  * Added support for function calls as select expressions in SPARQL.
  * Added support dots at the end of simple OPTIONAL statements in
    SPARQL.

  tracker-miner-fs:
  * Fixed crawling duplicate directories if one is in the config file
    and the other is a removable media.
  * Fixed memory leak with returned removable devices from storage (HAL).
  * Updated the Dia documentation for tracker-indexer to detail how
    tracker-miner-fs works.
  * Fixed so we don't print "miners all finished" more than once.

  tracker-extract:
  * Added tracker_text_normalize() to clean up content used for FTS.
    This function effectively strips non-text characters, extra spaces,
    carriage returns, etc.
  * Added FTS support for MS and ODF document formats.
  * Fixed PDF extractor to use tracker_text_normalize().

  tracker-tag:
  * Added support for removing a tag on a per URN basis instead of
    for all content.
  * Fixed return value

Bugs:

  * Fixes GB#570608 - Translation of firefox-extension
  * Fixes GB#592400 - String translation issues
  * Fixes GB#589059 - Add option to set evolution plugins directory in configure
  * Fixes GB#591814 - Clean up .ui files
  * Fixes GB#581984 - Get rid of deprecated libgnome(ui)
  * Fixes GB#595752 - Minor string fixes
  * Fixes NB#142257 - Query from QtTracker causes error

Notes:

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7.1
============

Improvements / New:

  General:
  * Updated version of dbus and dbus-glib and removed
    SUBJECT_TO_CHANGE use everywhere.
  * Updated version of libxml to something realistic.
  * Updated version of libgee to 0.3.
  * Updated vala requirement suggestion in autogen.sh to 0.7.6.

  Ontology:
  * Added prefix "mlo" for http://www.tracker-project.org/temp/mlo
  * Updated scal ontology to cover ical standard.

  Documentation:
  * Added initial documentation for libtracker-miner.
  * Fixed typo in tracker-processes man page.
  * Fixed warnings when building documenation for libtracker-common
  * Regenerated documentation sections for libtracker-common

  libtracker-common:
  * Fixed memory leak in tracker_string_to_date().
  * Moved TrackerParser API to libtracker-fts, only used there.

  libtracker-db:
  * Updated headers so only one header can be included if used externally

  libtracker-data:
  * Added support for fn:contains and fn:ends-with for SPARQL.
  * Removed unnecessary FTS INSERT and DELETE statements.
  * Updated to avoid unnecessary SQL CAST expressions.

  libtracker-fts:
  * Added to unit tests
  * Removed broken and unused paragraph detection.
  * Moved this from "tracker-fts" everywhere in code base since it is
    a library not an application.

  libtracker-miner:
  * Added get_status() and is_paused() APIs.
  * Removed GetName and GetDescription DBus methods, now in desktop
    files and exposed through TrackerMinerManager API.

  tracker-store:
  * Clean up queued batch updates to the database when clients
    disconnect.
  * Do not use JOIN for simple OPTIONAL patterns in SPARQL
    (significant speed improvement).

  tracker-miner-fs:
  * Added support back for thumbnailing.
  * Added support for full paths in config for IgnoredFiles and
    IgnoredDirectories.
  * Added support for XDG spec. for special user directories. So now
    in the config, IndexRecursiveDirectories and
    IndexSingleDirectories can include "&DOCUMENTS", or "&MUSIC", etc.
    The list of special locations is listed in the config and
    documented. This falls back to $HOME when not available for some
    distributions.
  * Removed unused files.
  * Updated the default indexed locations to DESKTOP, DOCUMENTS,
    DOWNLOADS, MUSIC, PICTURES and VIDEOS as per the XDG spec (this
    was $HOME).
  * Fixed trying to send thumbnail queues of size 0.
  * Fixed trying to send thumbnails when service is not available.
  * Fixed double unref in thumbnailer code.
  * Fixed 2 memory leaks found while valgrinding.

  tracker-extract:
  * Fixed assertion in HAL is NULL from starting on the command line.
  * Fixed Ctrl+C not working with -d.

  tracker-status:
  * Use libtracker-miner APIs instead of DBus calls.
  * Use libtracker-miner API to get display names (translatable).

  tracker-search-bar:
  * Fixed crash when clicking on the results window with the "no
    results found..." message.

  tracker-search-tool:
  * Added smart wildcards when last character is not a space.
  * Fixed category selector.
  * Fixed crasher when selecting categories.

Bugs:

  * Fixes NB#141309, Remove nco:hasPostalAddress cardinality limitation
  * Fixes NB#132655, Tracker-import error handling jams
  * Fixes GB#595976, Message 'SQL logic error or missing database'
  * Fixes GB#592400, String translation issues
  * Fixes GB#595752, Minor string fixes

Notes:

  The Deskbar applet packaged with Tracker is currently out of date
  and needs some love to get it working with the new APIs. It is in
  the same state as it was for 0.6. As such it isn't expected to work.


NEW in 0.7
==========

Improvements / New:

  * Library: libtracker-miner
    A common library for data miners to use allowing easy integration
    with Tracker. This provides D-Bus integration, state and progress
    APIs and a means to pause / resume your miner.

  * Library: libtracker-client
    This API has been completely changed. It is much simpler now and
    relies more on the SPARQL language than specific APIs to query
    Tracker.

  * Daemon: tracker-store
    Completely reworked to accept SPARQL queries and updates.
    Implements Nepomuk ontology instead of flawed own brand. This
    ontology is implemented following the W3C RDF semantics.

  * Database: Completely redesigned ALL databases.
    These now support triple store and allow much faster querying.

  * Application: tracker-miner-fs
    Implements libtracker-miner and facilitates the old roles of
    tracker-indexer and some of the old roles from trackerd like file
    system crawling. This mines data and sends it to tracker-store to
    put in the database about files AND applications.

  * Application: tracker-sparql
    A command line tool to query or update the databases using SPARQL.

  * Application: tracker-import
    A command line tool to import data kept in Turtle files.

  * Application: tracker-search-bar
    An applet for quickly searching for music, videos, images,
    documents and folders.

  * Application: tracker-status-icon
    Previously tracker-applet, but rewritten and now much simpler.
    This provides state and progress information about all miners
    which are running. This also allows the user to pause / resume
    data miners.

  * Application: tracker-search-tool
    This has been completely rewritten using vala and the new
    architecture underneath it. Support for the old features are not
    quite there yet, but it is similar to its predecessor!

  * Application: tracker-explore
    This is an application written to explore data in the database and
    each item's relationship to the next. This really demonstrates
    Tracker's ontology and provides it through full text searching.

  * Plugin: Evolution
    Now the evolution plugin is supported by the developers instead of
    us dipping into their databases (as previously) which is dangerous
    and unsafe. Also this implements the libtracker-miner API so it
    can be controlled using tracker-status-icon.

  * Patches: Nautilus & GtkFileChooser
    These are currently submitted upstream to support the new APIs for
    continued integration between Tracker and GNOME.

Bugs:

  Too many to list here and so much development some are even obsolete.

Notes:

  There is currently no migration path from 0.6. to 0.7. So all
  configurations will have to be re-set up for now. This is planned to
  be added before a 0.8 release.

  Man pages have all been revamped providing information about all new
  commands too.


NEW in 0.6.93:
==============
Improvements:
  * Fixes tracker-preferences so it mangages removable-devices option.
  * Fixes tracker config typo.
  * Fixes tracker-indexer to not enter finished state after flushing.
  * Fixes tracker-extract not sending queued album art to thumbnailerd
  * Fixes trackerd to call nice(19) for crawling ONLY else nice(default).
  * Fixes libtracker-common preprocessor checks for thumbnailer/albumart.
  * Fixes tracker-extract multiple TrackerHal instances, 2x faster now.
  * Fixes tracker-extract tests by loading modules not directly linking.
  * Fixes tracker-applet by adding proper marshal generation for signals.
  * Fixes SIGSEGV for tracker-extract from exceeding setrlimit() mem.
  * Fixes Raptor dependency to 1.4.17, need _FEATURE_WRITE_BASE_URI.
  * Fixes indexer flush time to be optimized for Maemo platform, patch 09.
  * Fixes gstreamer extractor memory leaks, a lot of leaks.
  * Fixes gstreamer extractor memory corruption.
  * Fixes gstreamer extractor by stopping extraction on DURATION messages
  * Fixes album art memory management and access() to NULL paths.
  * Fixes adding BUILT_SOURCES to EXTRA_DIST for many Makefile.ams
  * Fixes not using plural translations in tracker-info.
  * Fixes several memory leaks/corruptions in gstreamer/mp3/png extractors.
  * Fixed memory leak in indexer's removable-device module.
  * Fixed memory leak in indexer's push module, close the dir.
  * Fixed TrackerDBIndex _update_word() memmove outside boundries.
  * Added error handling for "Can't store word" to applet.
  * Added new API tracker_metadata_get_multiple().
  * Added pre-umount check there's an item before checking mount files.
  * Updated tracker-info to use _metadata_{get|_multiple}() APIs.
  * Updated extractors to use tracker_file_{open|close} with posix_fadvise
  * Removed all references to file-index-update.db, no longer used.
  * Improved performance by sending items to thumbailer AFTER indexing
  * Improves performance/mem of GFile->GQueue in tracker-monitor.c

Bugs:
  * Fixes GB#577656, tracker doesn't build on non-linux platforms
  * Fixes GB#577655, tracker-indexer.c fails to compile when...
  * Fixes LP#355982, crash in tracker-turtle.c with fclose()
  * Fixes NB#107570, tracker-extract crash just after device bootup
  * Fixes NB#106639, Music-Unknown Category  displayed in Genre view
  * Fixes NB#109172, Video Library: RM , RMJ files listed in Video Library.
  * Fixes NB#107122, Video:Duration is shown as -:- in video library view
  * Fixes NB#108793, Not able to filer images with XMP date
  * Fixes NB#108819, tracker-indexer is crashing.
  * Fixes NB#108331, Memory Leak found for tracker-extract command
  * Fixes NB#106085, Tracked database is not updated when images were...
  * Fixes NB#100642, tracker is still listing the removed tags.

Translations:
- Updated es: Jorge González, Jorge Gonzalez
- Updated it: Milo Casagrande
- Updated nl: Reinout van Schouwen

Help Manual Translations:
None


NEW in 0.6.92:
==============
Improvements:
- Fixed memmove error in QDBM when updating a word
- Fixed GValues leak in _result_set_to_metadata()
- Fixed small ontology error for image extractors
- Fixed trackerd not changing throttling when switching between charging/discharging states
- Fixed battery remaining percentage in logs, was always 0%
- Fixed crasher when calling thumbnailer with older versions of DBus, use int instead of int64
- Fixed logic for signalling status in tracker-indexer
- Fixed mtime checking in tracker-indexer, only reindex if file mtime is different, not just newer
- Fixed tracker-extract -f command to transform filenames to full paths
- Fixed setting tags in tracker-indexer
- Improve tracker-dbus_query_result_to_ptrarry(), don't recheck UTF8 validity of strings
- Improve failure conditions if RDF type doesn't exist in ontology
- Added GStreamer helix bin information to configure statement
- Added libtool library versioning so they are not all named .so.0.0.0.
- Added GError handling to database manager so we know why ontology files fail to load
- Added initial code to use GVolumeMonitor::monitor-pre-unmount, GIO patch is pending, was not implemented
- Added collations to Services table and other indices
- Added more states to tracker-applet to give more feedback, now we have watching, crawling, optimising and shutting down
- Added tracker-processes utility to list and kill tracker processes, this also has a --hard-reset option to remove all databases too
- Added a GetPid API to tracker-extract so we request the PID before doing anything
- Added kill() call to tracker-indexer when tracker-extract doesn't respond to the DBus request for metadata
- Added cache to trackerd for GetStats calls to be more responsive when indexing and if recently requested
- Removed call to set SCHED_IDLE, performance was way to bad to use it
- Removed Merge options from tracker-applet, not longer used
- Disabled static libraries by default
- Disabled evolution module since it is problematic and lots of Evolution issues were reported
- Enabled evolution push plugin (different to the Evolution module), non-intrusive, Evolution sends us the data
- Updated help strings for configure options of mail push modules
- Updated GetStats SQL stored procedure call to also include super classes like "Files"

Bugs:
- Fixed NB#106201, tracker-extract is crashing when extracting the details of a mp3 file (Mikael Ottela)
- Fixed NB#105133, Mafw-tracker-source does not emit container-changed signal (Martyn Russell)
- Fixed NB#90455,  Tracker doesn't index Geotags metadata Iptc4xmpCore:Location, photoshop:Country, photoshop:City of JPEG and PNG pictures (Mikael Ottela)
- Fixed NB#96314,  libexempi adds Creator in three different locations on ARM platform (Mikael Ottela)
- Fixed NB#99823,  Album art image gets corrupted if Artist and Album have characters other than alphabets (Mikael Ottela)
- Fixed NB#102800, Crash corruption code (Martyn Russell)
- Fixed LP#347352, Use g_strcmp0() to fix g_list_find_custom() crashing with NULLs (Martyn Russell)
- Fixed GB#106424, Crash when themes are deleted while running (Mikael Ottela)
- Fixed GB#575332, crashes when showing statistics (Martyn Russell)
- Fixed GB#570232, SetOption uses sqlite REPLACE which *doesn't* replace, it INSERTs (Carlos Garnacho)
- Fixed GB#573834, g_timeout_add_seconds should be preferred to g_timeout_add (Marcel Stimberg)
- Fixed GB#575271, Confusion and error with deskbar-applet module dir detection (Deji Akingunola)

Translations:
- Updated da: Mads Lundby
- Updated es: Jorge González, Jorge Gonzalez
- Updated gl: Suso Baleato
- Updated sv: Daniel Nylander

Help Manual Translations:
None

NEW in 0.6.91:
==============

Improvements:
- Fixed trackerd 'ServiceStatisticsUpdated' signal.
- Fixed a crash when removing a resource from a removable device.
- Fixed test case failures for MP3 files.
- Fixed subsequent album art requests trying if already downloading
- Fixed support for SCHED_IDLE.
- Fixed config so we make sure we load defaults for missing options.
- Fixed warning in indexer about thumbnailer delete action reply.
- Fixed regression, use "/" on end of generated URI paths for raptor.
- Fixed leaking file descriptors thought to cause "too many open files".
- Fixed negative underflow with gint16 value clamp in TrackerDBIndex.
- Fixed indexer corner case to flush when an item has been removed.
- Fixed .xsession-errors being populated with startup version info
- Fixed small regression in group concatenation db function.
- Fixed overwrite existing logs if already running (indexer/extractor).
- Fixed QDBM statistics for deleted items.
- Fixed deskbar so we find the right DBus service names.
- Fixed URI creation in indexer removable device module.
- Fixed leaks in indexer removable device module.
- Fixed MP3 frame size calculation.
- Fixed log handling, don't overwrite existing logs if already running.
- Fixed indexer responsiveness for ttl restores by iterating main loop.
- Fixed tracker-indexer reporting elapsed times suriously.
- Fixed tracker-indexer memory leak when retrieving embedded metadata.
- Fixed compiler warnings.
- Fixed problems in joining of order fields.
- Fixed issue with integer to string conversion in the database.
- Fixed tracker-extract blocking indefinitely for buggy modules.
- Fixed GetStats to check mounted volumes and to use SQL COUNT function.
- Fixed TrackerDBIndex so words with negative weight are actually indexed.
- Fixed indexer not shutting down and stopping before it should.
- Fixed indexer to reset indexed/processed stats after finishing.
- Fixed standard monitored directory list to omit .games/
- Fixed Makefiles which were missing WARN_CFLAGS.
- Fixed daemon so it now monitors disk space (was in indexer).
- Fixed daemon so it now monitors battery percentage (was in indexer).
- Fixed daemon state machine so we ALWAYS log state changes.
- Fixed daemon state machine so we don't return Initializing after index.
- Fixed album art so we don't add the artist name to the request.
- Fixed extractor messaging so we use debug statements for metadata.
- Fixed indexer to delete content for removed services.
- Fixed album art local URI not always being passed.
- Added Image:HasKeywords field for performance.
- Added warnings when we try to write to QDBM after opening as READONLY
- Added DB versions for vstore and 0.6.91 which force reindex if different.
- Added org.freedesktop.Tracker.Backup DBus interface.
- Added collation locale startup check and regeneration on change.
- Added GCC gcov support.
- Added new DBus function to get concatenated metadata group fields.
- Added new DBus function to Backup metadata to Turtle file.
- Added support for coverage reporting with gcov.
- Added IPTC support for TIFF and JPEG extractors
- Added dependency for libiptcdata.
- Added a configure switch to disable decompressing .ps.gz files to /tmp.
- Added initial push plugin support for KMail and RSS.
- Added NULL support to outer joins for conditions with empty strings.
- Added ModuleConfig option to ignore directories based on contents.
- Added album art code to libtracker-common for general use.
- Removed XESAM support completely.
- Removed all unused libtracker-common file functions.
- Removed a lot of unused code after adding WARN_CFLAGS.
- Removed duplicate calls to g_strstrip() for metadata in extractor.
- Removed unused extractors exif and gstreamer-playbin.
- Reduced the amount of debugging produced by HAL module.
- Reworked the src/plugin infrastructure to be more generic.
- Improved speed by using the cache for recursive deletes in the daemon.
- Improved speed by not setting DB page sizes so SQLite decides for us.
- Improved speed by removing UTF8 collation from Services table.
- Improved speed by making indexer flushing work asynchronously.
- Improved speed by making indexer flush frequency variable based on queue.
- Improved database GROUP_CONCAT aggregate function.
- Improved Backup/Turtle support.
- Improved indexer flush time from 60 seconds to 30 seconds, seems faster.
- Improved daemon dbus request pause time from 10 seconds to 5 seconds.
- Improved tracker-status so we can now follow real time state updates.
- Improved tracker utilities to g_print() a note if --limit is reached.
- Improved tracker utilities to use plural based translations.
- Improved tracker-services to sort properties listed.
- Updated extractors to use decodebin not playbin for speed.
- Updated project wide calls for mtime/size to use GIO not g_lstat().

Bugs:
- Fixed NB#105133, Mafw-tracker-source does not emit container-...
- Fixed NB#94201, NP-Audio:Album art not displayed.
- Fixed NB#96314, libexempi adds Creator in three different locations...
- Fixed LP#339560, buffer overflow getting file content.
- Fixed NB#102246, Container name is ‘blank’ within Artists/Albums...
- Fixed NB#103260, Empty 'Unknown' containers under Artists and Album...
- Fixed NB#103198, Unknown Category  displayed in Genre view
- Fixed NB#102210, Number of images in tracker response depends on...
- Fixed NB#91158, Restore factory settings is not working.
- Fixed NB#99823, Album art image gets corrupted if Artist and Album...
- Fixed NB#99827, Tracker is listing the files from memory card even after...
- Fixed NB#102800, Crash corruption code
- Fixed NB#102606, Tracker doesn't get duration of some MP3 filex
- Fixed NB#102167, ML: Songs deleted from MMC shown in Library
- Fixed NB#98297, tracker-stats values are not changed even after deleting...
- Fixed NB#101593, images created from sketch application are listed...
- Fixed NB#101142, Sorting by Image:Date is incorrect
- Fixed NB#95573, Keep quiet, we're in a call
- Fixed GB#102606, [patch] NetBSD: gconf-internals.c: `AF_INET' undeclared (Mikael Ottela)
- Fixed GB#105133, With a 3-monitor xinerama, window placement bad (Martyn Russell)
- Fixed GB#103260, Session script setup that works with multiple *dm and supports a switchdesk-type feature etc. (Mikael Ottela)
- Fixed GB#101593, FSF copyright notice wrong in gnome-panel panel (Mikael Ottela)
- Fixed GB#103198, gtk_tree_store_move_* doesn't work. (Mikael Ottela)
- Fixed GB#455588, indexing of Abiword templates slow (Carlos Garnacho)
- Fixed GB#489876, Unknown creation date for pdf files (Carlos Garnacho)
- Fixed GB#503595, check/uncheck "Genertate thumbnails" doesn't work (Carlos Garnacho)
- Fixed GB#101096, Save as Excel spreadsheet version 97 (Mikael Ottela)
- Fixed GB#572237, Remove deprecated Glib symbols (Carlos Garnacho)
- Fixed GB#571331, wrong range for throttling slider (Martyn Russell)
- Fixed GB#571573, Tracker's deskbar module has incorrect import (Michel Alexandre Salim)
- Fixed GB#101142, make breaks without xft (Mikael Ottela)
- Fixed GB#101296, Cannot find pixmaps balsa_icon.png and attachment.png (Martyn Russell)
- Fixed GB#102800, xlstproc 1.0.23 fails to create directories using -o (Martyn Russell)
- Fixed GB#519222, trackerd crashes when system dbus is restarted/stopped (Carlos Garnacho)
- Fixed GB#571736, Get rid of deprecated libart_lgpl


Translations:
- Updated da: Mads Lundby
- Updated es: Jorge González, Jorge Gonzalez
- Updated hu: Gabor Kelemen
- Updated it: Luca Ferretti
- Updated pt: Nicolau Gonçalves, Nicolau Gonçalves <excentriko@gmail
- Updated sv: Daniel Nylander

Help Manual Translations:
None



NEW in 0.6.90:
==============

Features:
- Major re-factoring over the last 12 months for cleaner code.
- Modular approach with:
  - libtracker-common
    (common functions shared across all binaries)
  - libtracker-db
    (database abstraction and manipulation API)
  - libtracker-data
    (a higher level data get/set API)
  - libtracker-module
    (an API used by 3rd parties to index their content)
- DBus re-factoring and use of GLib bindings (for maintainability).
  (The method names haven't changed, but the objects have, so the
  client applications will need to be updated).
- Extractors are now run as an external process detached from the
  indexer to not crash the indexer for problematic files.
- The utility applications have all been improved and use GOption
  for argument parsing.
- Configuration is now more centralised and used by applications as
  well as the daemon/indexer.
- File monitoring has been rewritten using GIO and libinotify:
- GnomeVFS support dropped from the project.
- Requirement for GLib 2.16.x.
- Imported libinotify (for use in the daemon for file monitoring).
  (due to lack of MOVE event information from GIO).
- Support for DBus introspection.
- Support for removable media indexing (MMC, USB sticks).
- Support for meta-data caching on removable devices
  (see http://live.gnome.org/MetadataOnRemovableDevices)
- Support for searching with OR operations.
- Support for XESAM (initial support only).
- Added application dedicated to indexing (tracker-indexer).
- Added estimated time remaining while indexing.
- Added module API and framework for applications to add support for
  their media files to be indexed.
- Added utility applications:
  - tracker-unique
    (searches for unique values with option RDF filters)
  - tracker-services
    (lists all ontology services/properties)
  - tracker-info
    (lists details known to tracker about a particular file)
- Added unit tests for:
  - trackerd
  - tracker-indexer
  - tracker-extract
  - libtracker-common
  - libtracker-db
- Added documentation for:
  - daemon/indexer design
  - libtracker-common
  - libtracker-module

Missing Support:
- Index merging has been removed in this release due to lack of
  time to implementing it and with plans to redesign. SQLite FTS has
  the feature internally and will be used in future releases. This
  is noticeable only for very large data sets where a slowdown can be
  observed.

Bugs:
- Fixed GB#486733, crash in Deskbar: Trying to launch the Tra... (Carlos Garnacho)
- Fixed GB#474131, crash in Deskbar: Deskbar search applet --... (Carlos Garnacho)
- Fixed NB#97029, Typos in the italian translation of the About box (Mikael Ottela)
- Fixed NB#97765, GFlare: switchng between circle/poly doesn't work (Ivan Frade)
- Fixed NB#93935, line object connects to self (Ivan Frade)
- Fixed NB#90700, "View as EOG Image" (Martyn Russell)
- Fixed NB#90138, no way to cleanly unset a message's message_id header (Mikael Ottela)
- Fixed NB#96999, Right-click context menu should contain copy entry (even on webpages without input fields) (Mikael Ottela)
- Fixed GB#100816, [ui-review] Edit menu usability (Mikael Ottela)
- Fixed NB#90374, help crashes (Mikael Ottela)
- Fixed NB#93962, gtkspinbutton wants to own its adjustment (Philip Van Hoof)
- Fixed NB#97125, missing dot between two sentences in docs for gtk_icon_size_lookup_for_settings () (Mikael Ottela)
- Fixed NB#89254, Documentation updates and corrections for GtkTreeModel (Mikael Ottela)
- Fixed NB#99281, acme should use an event filter (Mikael Ottela)
- Fixed NB#92788, add fcpackage to bootstrap (Martyn Russell)
- Fixed GB#402054, Should not allow duplicate keywords to be assigned to a file (Ivan Frade)
- Fixed GB#402063, Empty space is not a valid keyword (Ivan Frade)
- Fixed GB#441739, Hit count not updated after deleting files (item_remove)
- Fixed GB#481651, Minor spelling issue (Carlos Garnacho)
- Fixed GB#493049, Trailing space in an original message in the message catalog (Ivan Frade)
- Fixed GB#494878, Search hit number can not be reported by orca. (Saleem Abdulrasool)
- Fixed GB#494907, Labels should be set as focusable. (Saleem Abdulrasool)
- Fixed GB#495810, Executing a wrong RDF query makes tracker unresponsive and unstable (Carlos Garnacho)
- Fixed GB#509601, Make metadata extractors modular (Carlos Garnacho)
- Fixed GB#509607, Make Search support logical operators besides 'And' (Carlos Garnacho)
- Fixed GB#511781, Patch to avoid excesive relocations (array_to_list)
- Fixed GB#511788, Patch to simplify tracker_exec_proc_ignore_nulls() (Carlos Garnacho)
- Fixed GB#513486, TrackerConfig implementation (Martyn Russell)
- Fixed GB#513691, simplify tracker_exec_proc() (Saleem Abdulrasool)
- Fixed GB#513692, Pack booleans in guints (Saleem Abdulrasool)
- Fixed GB#514553, Implementing GetTextContents method (Martyn Russell)
- Fixed GB#518265, Bad FSF address (Ivan Frade)
- Fixed GB#523200, Refactoring Service type "cache" (Ivan Frade)
- Fixed GB#525403, [Refactor] Isolating indexer (Ivan Frade)
- Fixed GB#526727, Parsing dates in extractors (Ivan Frade)
- Fixed GB#526761, Make mail indexing modular (Carlos Garnacho)
- Fixed GB#528684, [Tracker] The second Applet Preferences window can not be closed (Wendy Hu)
- Fixed GB#529373, Use logging infrastructure from GLib (Carlos Garnacho, Martyn Russell)
- Fixed GB#548561, Typo in trackerd man page (Ivan Frade)
- Fixed GB#557979, tracker-preferences.desktop categories (Ivan Frade)
- Fixed GB#558613, Applet preferences do not show up localized (Gabor Kelemen)
- Fixed GB#560581, [l10n] Typo in Tracker message (Ivan Frade)
- Fixed GB#563666, All tracker utils programs require absolute paths (Ivan Frade)
- Fixed GB#563909, Canonicalize command line options (Luca Ferretti)
- Fixed GB#565218, GNOME Goal: Clean up GLib and GTK+ includes (Luis Menina, Ivan Frade)
- Fixed GB#565725, Please refactor set_status_hint in tracker-applet.c (Ivan Frade)
- Fixed GB#565928, tracker-applet --help does not show up translated (Ivan Frade)
- Fixed GB#565932, Some strings are not marked for translation (Ivan Frade)
- Fixed GB#566221, nautilus python extension tracker-tags-tab.py does not work in trunk anymore (Thomas)
- Fixed GB#556196, Include <sys/inotify.h>, not <linux/inotify.h> (John Millikin)
- Fixed GB#495512, Tracker Deskbar Applets crashes (Carlos Garnacho)
- Fixed GB#511752, Build fixes for Maemo platform (Martyn Russell)
- Fixed GB#520789, Build compiler warnings (Martyn Russell)
- Fixed GB#521803, Refactor request and file processing (Martyn Russell)
- Fixed NB#95754, v1.03 Compilation impossible avec bison 1.50 (Ivan Frade)
- Fixed NB#95151, Keybinding reported for a button should include the modifer. (Ivan Frade)
- Fixed NB#90455,  (Mikael Ottela)
- Fixed GB#494892, Label for/labeled by relation missing for add tag button. (Saleem Abdulrasool)
- Fixed GB#495201, Tables are not labeled. (Patrick Wade)
- Fixed GB#495205, Warning message label should be accessible to at-tools. (Saleem Abdulrasool)
- Fixed GB#503215, The time unit is not reported by default. (Saleem Abdulrasool)
- Fixed GB#563706, Tracker plugin fails to build against trunk (Martyn Russell)
- Fixed GB#570209, tracker_data_manager_set_db_option_int() uses incorrect parameter order for SetOption (TJ)
- Fixed GB#495528, "Save Results As" doesn't work for Email results (Carlos Garnacho)
- Fixed GB#503725, check/uncheck "Index mounted directories" needs trackerd reindex (Martyn Russell)
- Fixed NB#98896, ru.po still in KOI8 (Martyn Russell)

Translations:
- Updated ar: Khaled Hosny
- Updated be: Alexander Nyakhaychyk
- Updated be@latin: Ihar Hrachyshka
- Updated ca: Gil Forcada
- Updated cs: Petr Kovar, Kamil Paral
- Updated da: Mads Lundby
- Updated el: Simos Xenitellis
- Updated en_GB: Philip Withnall, David Lodge
- Updated es: Jorge González, Jorge Gonzalez
- Updated fi: Timo Jyrinki, Ilkka Tuohela
- Updated fr: Marc Veillet, Robert-André Mauchin and Stéphane Raimbault
- Updated hu: Gabor Kelemen
- Updated it: Luca Ferretti
- Updated ja: Takao Fujiwara, Takao Fujiwara <takao
- Updated lt: Žygimantas Beručka, Gintautas Miliauskas
- Updated nb: Kjartan Maraas
- Updated nl: Roel Huybrechts
- Updated oc: Yannig Marchegay (Kokoyaya)
- Updated pl: Tomasz Dominikowski, Wadim Dziedzic
- Updated pt: Nicolau Gonçalves, Nicolau Gonçalves <excentriko@gmail
- Updated pt_BR: Henrique P Machado, Henrique Machado and Fábio Nogueira
- Updated sk: Pavol Klačanský
- Updated sv: Daniel Nylander
- Updated th: Manatsawin Hanmongkolchai, Isriya Paireepairit
- Updated zh_CN: Martyn Russell, YangZhang

Help Manual Translations:
None



Tracker 0.6.6
============= (03 Mar 2008)

  Applet
  * HIG fix for preference window.
  * Progress for index merges added.

  Tracker daemon
  * Fixed shutdown of daemon when paused to prevent hangs.
  * Fixed an unfinished final index merge step when next restarted.



Tracker 0.6.5
============= (27 Feb 2008)

  Applet
  * Removed popup window and replaced with tooltip displaying status
    and progress.
  * New Applet preferences to turn off animation/hide icon/set
    smart pause.
  * Smart pause feature which can auto-pause the Tracker daemon
    whenever the user presses a key or moves the mouse, thereby
    improving system responsiveness during indexing.
  * Annoying merge messages have now been removed.
  * Reindex option added.
  * About box added.
  * Now hidden if indexing is disabled.
  * Bugs with constant animation fixed.
  * Animation rate slowed right down so as to be less annoying.

  Tracker daemon
  * Temporary blacklisting of frequently changing files (they are
    indexed after one hour or on next restart of Tracker daemon).
    This prevents constant indexing from downloads and torrents.
  * More blacklisting of common tmp formats and vmware files.
  * Memory leak fixes.
  * Removed CPU-heavy corruption scan which can last long time and
    occurs on battery too at startup when tracker daemon was not
    previously shut down properly. Instead we now use bigger
    transactions with fsync on to prevent corruption in the first
    place (only slightly more disk IO is used during indexing as a
    result).
  * Fixed crashers.
  * Fixed D-Bus shutdown/reindex call.



Tracker 0.6.4
============= (11 Dec 2007)

  * Made indexing more robust by pausing if disk space is low or
    index grows too big.
  * Limit log file size to 10MB.
  * New Tracker Applet - animates when indexing, provides ability to
    pause indexing as well as viewing status and progress feedback
    from indexer, statistics, and notification warnings; it can
    also launch Tracker Preferences and Tracker Search Tool.
  * New power management options enable much better customization.
  * Ignored files fixes.
  * Deskbar/tracker integration fixes.
  * Made most prefs live and affect Tracker in real time; others
    will prompt for restart and/or reindex where necessary.
  * Shell script fixes
  * Fixed Imap bug with embedded Auth in URI.
  * Built-in corruption check and scan when Tracker is not shut down
    cleanly - prevents infinite looping.
  * Fix index deletions.
  * Many more bug fixes and stability improvements.



Tracker 0.6.3
============= (25 Sep 2007)

  * Fixed memory leaks.
  * New index merging which allows for much better performance and
    much less disk hogging I/O.
  * Battery throttling - Tracker now stops indexing when on battery.
  * Many more optimisations - transactions, use of O_NOATIME and
    posix_fadvise everywhere.
  * Check files exist before including in search results.
  * Fixed disabling of indexing option in tracker-preferences.
  * HIG fixes to tracker-preferences screen.
  * Fixed resetting of stats.

  note:
    This version will cause your hard drive to be re-indexed so
    that stats are fixed (previous version broke them).



Tracker 0.6.2
============= (25 Sep 2007)

  * New Sqlite-based indexer which utilises the new incremental blob
    I/O in sqlite 3.4.
  * Highly-optimised email indexing (up to 5x faster).
  * Dramatically reduced disk access and disk contention.
  * Indexer now pauses for a grace period when non-Tracker processes
    write to disk (providing changed files are being watched by
    Tracker) - minimises slowdowns when compiling or checking out
    source code.
  * Makes use of idle class disk IO scheduling if available.
  * Makes preliminary use of NO_ATIME (some disk access still uses
    fopen).
  * Fixed restore of user metadata on re-index (keywords are
    auto-restored).
  * Added increased number of (junk) files to automatically ignore.
  * Improved stopwords.
  * New Deskbar handler that uses the new Deskbar api (2.19+).
  * Fixed old Deskbar handler to remove race condition causing
    crashes.
  * Fixed a number of annoying bugs in email indexing and Tracker
    Preferences.

  note:
    This version will cause your hard drive to be re-indexed due to
    the new Sqlite indexer backend.


Tracker 0.6.1
============= (08 Aug 2007)

  * translations and bugfixes


Tracker 0.6.0 "better late than never"
============= (24 Jul 2007)

  * Evolution Emails (mbox/pop/imap) now indexed.
  * Gaim/Pidgin chat logs now indexed.
  * Applications now indexed.
  * New files indexed - all spreadsheets, csv (these require
    Gnumeric to be installed) and oo draw.
  * Vastly improved and new search UI using a sidebar for categories
    and a new metadata tile.
  * Tagging support in the UI.
  * New tracker-preferences UI for easy setting of prefs.
  * Added libtracker-gtk widget toolkit.
  * Now uses XDG directories.
  * Optimised triple store.
  * Handles multiple triple stores (files, emails and user data are
    all separate dbs).
  * New IDF based ranking algorithm.
  * XMP metadata support added.
  * Made use of async calls in tracker-search-tool to prevent it
    from freezing.
  * Added a 60 second delay before indexing to prevent slowdowns on
    desktop start-up.
  * Added support for crawling directories without watching overhead
    (useful for indexing source code without causing slowdowns when
     compiling).


Tracker 0.5.4 "Speed Daemon"
============= (26 Jan 2007)

  * Indexing at ludicrous speed - massively optimised indexing so
    its now 10x faster than previous version. Indexing speeds are
    now around 100 text files per second (which is about the
    maximum possible considering the I/O time to read 100 files from
    a hard drive).
  * Index while you work - now scheduled even better so it should
    never slow the system down whilst allowing users to work without
    being affected by or even noticing it.
  * Also provides additional --throttle command line parameter to
    customise throttling even further so no need to endure noisy
    fans or hot laptops while indexing.
  * New verbosity param to control how much data is shown on screen
    and in log.
  * Fixed MPlayer backend which is now used if gstreamer backend
    returns no metadata
  * Improved reaping of all spawned apps - no more zombies!
  * Improved mime and text file detection.
  * Added improved deskbar-handler.
  * Got Tracker to run nicely on FUSE based filesystems.


Tracker 0.5.3
============= (24 Dec 2006)

  * Fixed memory leaks when indexing (typical mem usage should now
    be ~6MB RSS when indexing large numbers of files).
  * New improved metadata engine now sports multiple values per
    metadata item and rdf style recursive metadata relationships.
  * Support for Dublin Core types (especially for metadata
    relationships).
  * Changed command line options for trackerd to be similiar to
    other programs.
  * Metadata names now to reflect rdf style syntax (Eg DC:Keywords).
  * Improved indexing of numeric data so that only meaningful
    numbers are indexed (IE must contain 5 or more consecutive
    numbers to be indexable).
  * Improved indexing of filenames so that hyphens and underscores
    are used as word breaks to create a delimited filename in
    addition to the full filename.
  * Improved snippet generator to start/end on a word break if
    possible.


Tracker 0.5.2
============= (21 Nov 2006)

  * Now has l10n/i18n support.
  * Added timeout to all external processes and metadata extractors
    used by Tracker.
  * Added missing OpenOffice templates to our filters.
  * Fixed filters to use different packages like w3m for indexing
    html.
  * Fixed Debian build and added new packages for data files and
    translations.
  * Imporved t-s-t and made snippets wordwrap, made snippets async,
    used Tracker's mime facility, improved highlighting and fixed
    crasher.
  * Limited tracker-extract's memory usage using setrlimit.
  * Added desktop files for tracker-search-tool.


Tracker 0.5.1
============= (07 Nov 2006)

  * Inlined a threadsafe version of sqlite to prevent common errors
    like content not being indexed. This can be overriden for those
    with a threadsafe version of SQlite.
  * New Gstreamer-based audio/video extractor on by default.
  * Made extractors more robust.
  * Fixed a number of compiler warnings.
  * Tidied up the tracker-search-tool to have buttons more evenly-sized
  * Fixed a few leaks and made the D-Bus interface more robust.
  * Imporved IOprio detection.


Tracker 0.5.0
============= (25 Oct 2006)

  * Completely new backend that has been moved from Mysql to the
    nimble Sqlite database and QDBM inverted word index.
  * New tracker-search-tool GUI which allows you to search for your
    files in a google-like fashion with results depicted with
    google-style search snippets.
  * Revamped Rdf Query engine.
  * New lighter design with big dependencies like libextractor
    having been dropped in favour of lighter and optional metadata
    extractors.
  * Added new extractors that can use Totem or MPlayer.
  * New options to tune Tracker for your system. We now have a
    --enable-low-memory setting for use with computers with less
    RAM and a --turbo mode for those  who want indexing to take
    place as fast as possible.
  * New snowball stemmers have been integrated into Tracker to
    provide some of the highest quality open source stemming around.
  * New language-specific stopword lists to prevent indexng of
    common words (which are of little value).
  * Can optionally make use of Pango word breaking for full
    internationalisation support.


Tracker 0.0.4 "Now indexing at Warp speed"
============= (16 May 2006)

  * Hugely optimised indexing when many files are waiting to be
    indexed (especially when you first run trackerd).
  * Mass queueing of files no longer blocks the main thread, meaning
    super fast searches can still be performed during heavy indexing.
  * Eliminated CPU bottlenecks and improved thread synchronisations
    so Tracker now hits the ground running when indexing (approx
    500+ files indexed per minute on inotify-enabled systems).
  * Extended metadata support for more Exif fields.
  * Added more service types.
  * Imporved build and support for FC5 (includes RPMs).
  * Redesigned Database around version 5 of the *embedded*
    in-process MySQL database library.
  * Now uses the auto-repair facility provided by MySQL to
    automatically repair damaged database files so you need never
    worry about losing your precious data.
  * Moved virtually all DB logic into stored procedures which
    provides a clean seperation of DB logic and application logic.
  * Added support for parsing dates in various formats including
    conversion to/from ISO 8601 format
  * Fixed MSWord filter to prevent looping (WvText causes inotify to
    report file write change causing endless looping).
  * Redesigned DB structure to be more generic and service orientated
  * Added support for service types (first class objects) to DB.
  * Cleaned up code warnings and fixed potential crasher.
  * Fixed issues with argv handling.
  * Fixed build issue for Fedora Core 5.


Tracker 0.0.3
============= (29 Mar 2006)

  * Made D-Bus interface more generic
  * Corrected search results of hyphenated search terms.
  * Added support for limiting no. of hits for searches.
  * Added optional support for sorting search results by relevance
    score.
  * A few minor bug fixes (Updating file name metadata correctly
    during move, correcting use of hyphenated search terms).
  * Streamlined the inlined version of libextractor and improved
    build system and fixed bug in previous version which prevented
    compilation of the inlined version.


Tracker 0.0.2
============= (27 Feb 2006)

  * Full inotify support (only compatible with Linux kernels 2.6.13
    or higher) with optimisations for detected moves.
  * Fallback to FAM/GAMIN and manual polling for non-Linux and/or
    non-inotify systems.
  * Improved build system which allows you to specify which file
    monitoring backend to use.
  * Valgrind'ed to remove all leaks.


16 Jan 2006
  *	Added support for Tracker as a Nautilus Search Backend.


05 Jan 2006
  * Added polling mechanism to watch files in absence of FAM/iNotify
	or when FAM/iNotify watch limits have been exceeded.

13 Dec 2005
  * Imported Tracker into GNOME CVS.


08 Dec 2005
  * Pre-release of Tracker launched.

15 Sep 2005
  * Tracker project started.

