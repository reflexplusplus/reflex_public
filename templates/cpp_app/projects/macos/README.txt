Local macOS paths

paths.xcconfig contains machine-specific paths used by the Xcode project.
It is intentionally ignored by Git so different developers and machines can use
their own Reflex installation and output locations without shared-path conflicts.

paths.xcconfig.RENAME is the checked-in starter file. On a new checkout, copy
or rename it to paths.xcconfig, then adjust the local values as needed.
