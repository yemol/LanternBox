from build_context_profiles import build as build_profiles
from build_benchmark import build as build_benchmark
from build_guides import build as build_guides

build_profiles()
build_benchmark()
build_guides()

print("\n✓ Build Finished")