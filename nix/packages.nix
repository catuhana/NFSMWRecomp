{
  llvmStdenv,
  nativeBuildInputs,
  buildInputs,

  fetchFromGitHub,
  nix-gitignore,

  gameXex ? null,

  ...
}:
let
  hostPlatform = llvmStdenv.hostPlatform;

  os =
    if hostPlatform.isLinux then
      "linux"
    else if hostPlatform.isDarwin then
      "macos"
    else
      throw "unsupported platform ${hostPlatform}";
  arch =
    if hostPlatform.isx86_64 then
      "amd64"
    else if hostPlatform.isAarch64 then
      "arm64"
    else
      throw "unsupported architecture ${hostPlatform}";

  mkPreset = buildType: "${os}-${arch}-${buildType}";

  dependencies = {
    nfd = fetchFromGitHub {
      owner = "btzy";
      repo = "nativefiledialog-extended";
      rev = "v1.3.0";
      hash = "sha256-JrwJP7zt/4oW4OQHCEM23k+zm6j1AVglGJowwkWc29k=";
    };

    cstring_view = fetchFromGitHub {
      owner = "bemanproject";
      repo = "cstring_view";
      rev = "bee112df1e994ab842556677d698428c3edbf713";
      hash = "sha256-a5FkHV8zxTq4Q8ucpmbzq/iWH0gKLit9bskyD493r7w=";
    };
  };
in
{
  default = llvmStdenv.mkDerivation {
    pname = "NFSMWRecomp";
    version = "0.0.0";

    src = nix-gitignore.gitignoreSource [ ] ../.;

    inherit
      nativeBuildInputs
      buildInputs
      ;

    dontUseCmakeConfigure = true;

    postPatch = ''
      substituteInPlace vendor/rexglue-sdk/thirdparty/CMakeLists.txt \
        --replace-fail 'if(NOT EXISTS "''${CMAKE_CURRENT_SOURCE_DIR}/''${submodule}/.git")' 'if(FALSE)'

      substituteInPlace vendor/rexglue-sdk/include/rex/chrono/chrono.h \
        --replace-fail '#ifdef __APPLE__' '#if defined(__APPLE__) || defined(_LIBCPP_VERSION)'
    '';

    configurePhase = ''
      runHook preConfigure

      ${
        if gameXex == null then
          ''
            echo "error: NFSMW_GAME_XEX is not set." >&2
            exit 1
          ''
        else
          ''
            mkdir -p apps/NFSMWRecomp/game
            cp ${gameXex} apps/NFSMWRecomp/game/default.xex
          ''
      }

      cmake --preset ${mkPreset "release"} \
        -DCMAKE_INSTALL_PREFIX=$out \
        -DFETCHCONTENT_SOURCE_DIR_NFD=${dependencies.nfd} \
        -DFETCHCONTENT_SOURCE_DIR_BEMAN.CSTRING_VIEW=${dependencies.cstring_view} \
        -DFETCHCONTENT_FULLY_DISCONNECTED=ON

      runHook postConfigure
    '';

    buildPhase = ''
      runHook preBuild

      cmake --build --preset ${mkPreset "release"}

      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall

      cmake --install build/${mkPreset "release"}

      runHook postInstall
    '';
  };
}
