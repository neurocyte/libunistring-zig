const std = @import("std");
const sources_musl = @import("musl/libunistring.sources.zig");
const sources_gnu = @import("gnu/libunistring.sources.zig");
const P = std.fs.path.sep_str;

fn thisDir() []const u8 {
    return std.fs.path.dirname(@src().file) orelse ".";
}
const package_dir = thisDir();

const flags = [_][]const u8{ "-DHAVE_CONFIG_H", "-DNO_XMALLOC", "-DIN_LIBUNISTRING", "-DDEPENDS_ON_LIBICONV=1" };

const Sources = struct {
    source_files: []const []const u8,
    include_path: []const u8,
    lib_include_path: []const u8,
    base_include_path: []const u8,
};

pub fn get_sources(src: anytype) Sources {
    return .{
        .source_files = &src.source_files,
        .include_path = src.include_path,
        .lib_include_path = src.lib_include_path,
        .base_include_path = src.base_include_path,
    };
}

pub fn build(b: *std.build.Builder) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const abi = target.abi orelse .gnu;

    const sources = switch (abi) {
        .gnu, .gnuabin32, .gnuabi64, .gnueabi, .gnueabihf, .gnuf32, .gnuf64, .gnusf, .gnux32, .gnuilp32 => get_sources(sources_gnu),
        .musl, .musleabi, .musleabihf, .muslx32 => get_sources(sources_musl),
        else => get_sources(sources_musl),
    };

    const lib = b.addStaticLibrary(.{
        .name = "libunistring",
        .target = target,
        .optimize = optimize,
    });
    lib.linkLibC();
    lib.addIncludePath(.{ .path = sources.include_path });
    lib.addIncludePath(.{ .path = sources.lib_include_path });
    lib.addIncludePath(.{ .path = sources.base_include_path });
    lib.addCSourceFiles(sources.source_files, &flags);

    b.installArtifact(lib);
    lib.installHeadersDirectory(sources.include_path, "");
}
