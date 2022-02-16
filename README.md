# build-tools
Tools for helping with building, packing &amp; signing software products

## Code signing steps

1. Sign app or plugin.


    codesign -s "Developer ID Application: YOURCOMPANY (TEAM_ID)" "path-to-file" --timestamp


2. Pack products into .pkg or .dmg.

3. Notarize.

    
    xcrun altool --notarize-app -f "path-to-packed-file.pkg" --primary-bundle-id APP_BUNDLE_ID --username "DEVELOPER_EMAIL" --password "APP_SPECIFIC_PASSWORD"

4. Wait for email with confirmation from Apple, and then staple.

    
    xcrun stapler staple "PATH_TO_FILE.PKG"

5. Check.


    spctl -a -vvv -t install "PATH_TO_FILE.PKG"
