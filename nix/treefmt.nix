_: {
  programs = {
    nixfmt.enable = true;
    taplo.enable = true;
  };

  settings.excludes = [ "vendor/**/*" ];
}
