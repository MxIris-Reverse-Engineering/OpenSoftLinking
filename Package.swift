// swift-tools-version: 6.3
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "OpenSoftLinking",
    platforms: [
        .macOS(.v10_15),
        .iOS(.v13),
        .tvOS(.v13),
        .watchOS(.v6),
        .visionOS(.v1),
        .macCatalyst(.v13),
    ],
    products: [
        .library(
            name: "OpenSoftLinking",
            targets: ["OpenSoftLinking"]
        ),
    ],
    targets: [
        .target(
            name: "OpenSoftLinking",
            publicHeadersPath: "include",
            cSettings: [
                .headerSearchPath("."),
            ]
        ),
        .testTarget(
            name: "OpenSoftLinkingTests",
            dependencies: ["OpenSoftLinking"]
        ),
    ],
    swiftLanguageModes: [.v6]
)
