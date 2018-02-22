Don't install picard.in

--- setup.py.orig	2018-02-17 17:56:58 UTC
+++ setup.py
@@ -793,6 +793,5 @@ if py2exe is None and do_py2app is False:
     args['data_files'].append(('share/icons/hicolor/256x256/apps', ['resources/images/256x256/picard.png']))
     args['data_files'].append(('share/icons/hicolor/scalable/apps', ['resources/img-src/picard.svg']))
     args['data_files'].append(('share/applications', ('picard.desktop',)))
-    args['data_files'].append('scripts/picard.in')
 
 setup(**args)
