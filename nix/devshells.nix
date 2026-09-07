{
  llvmStdenv,
  nativeBuildInputs,
  buildInputs,

  mkShell,

  nixfmt,
  nixd,
  ...
}:
{
  default =
    (mkShell.override {
      stdenv = llvmStdenv;
    })
      {
        inherit
          nativeBuildInputs
          buildInputs
          ;

        packages = [
          nixfmt
          nixd
        ];
      };
}
