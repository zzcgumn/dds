// swift-tools-version: 6.2
import PackageDescription

let package = Package(
    name: "dds",
    products: [
        .library(name: "dds", targets: ["dds"]),
    ],
    targets: [
        .target(
            name: "dds",
            path: "library/src",
            publicHeadersPath: "include"
        ),
    ],
    cxxLanguageStandard: .cxx20
)
