const std = @import("std");
const sources = @import("libunistring.sources.zig");

const P = std.fs.path.sep_str;

fn thisDir() []const u8 {
    return std.fs.path.dirname(@src().file) orelse ".";
}
const package_dir = thisDir();

const flags = [_][]const u8{ "-DHAVE_CONFIG_H", "-DNO_XMALLOC", "-DIN_LIBUNISTRING", "-DDEPENDS_ON_LIBICONV=1" };

pub fn build(b: *std.build.Builder) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const lib = b.addStaticLibrary(.{
        .name = "libunistring",
        .target = target,
        .optimize = optimize,
    });
    lib.linkLibC();
    lib.addIncludePath("include");
    lib.addIncludePath("lib");
    lib.addIncludePath(".");
    addSources(lib);

    lib.install();
    lib.installHeadersDirectory("include", "");
}

fn addSources(self: *std.build.CompileStep) void {
    for (sources.source_files) |file| {
        self.addCSourceFile(file, &flags);
    }
}
