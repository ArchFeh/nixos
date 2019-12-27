# Edit this configuration file to define what should be installed on
# your system.  Help is available in the configuration.nix(5) man page
# and in the NixOS manual (accessible by running ‘nixos-help’).

{ config, pkgs, ... }:

{
  imports =
    [ # Include the results of the hardware scan.
      ./hardware-configuration.nix
    ];

  # Use the systemd-boot EFI boot loader.
  # boot.loader.systemd-boot.enable = true;
  boot.loader.efi.canTouchEfiVariables = true;
  boot.loader.grub.enable = true;
  boot.loader.grub.device = "nodev";
  boot.loader.grub.efiSupport = true;
  
  boot.kernelPackages = pkgs.linuxPackages_latest;

  networking.hostName = "nixos"; # Define your hostname.
  networking.networkmanager.enable = true;
  # networking.wireless.enable = true;  # Enables wireless support via wpa_supplicant.

  # The global useDHCP flag is deprecated, therefore explicitly set to false here.
  # Per-interface useDHCP will be mandatory in the future, so this generated config
  # replicates the default behaviour.
  networking.useDHCP = false;
  networking.interfaces.enp3s0.useDHCP = true;
  networking.interfaces.wlp2s0.useDHCP = true;

  # Configure network proxy if necessary
  # networking.proxy.default = "http://user:password@proxy:port/";
  # networking.proxy.noProxy = "127.0.0.1,localhost,internal.domain";

  # Select internationalisation properties.
  i18n = {
  #   consoleFont = "Lat2-Terminus16";
  #   consoleKeyMap = "us";
  #   defaultLocale = "en_US.UTF-8";
    supportedLocales = [ "all" ];
    inputMethod.enabled = "fcitx";
    inputMethod.fcitx.engines = with pkgs.fcitx-engines; [ cloudpinyin ];
  };
  
  fonts.fonts = with pkgs; [
    wqy_microhei];
  # Set your time zone.
  time.timeZone = "Europe/Moscow";

  # List packages installed in system profile. To search, run:
  # $ nix search wget
  environment.systemPackages = with pkgs; [
    wget neovim curl git screenfetch typora tdesktop wpsoffice vk-messenger mpich
netease-cloud-music
    google-chrome firefox autojump gnumake vscode pciutils 
    konsole dolphin sddm-kcm latte-dock spectacle
    fcitx-configtool fcitx-engines.cloudpinyin 
  ];
  nixpkgs.config.allowUnfree = true;
  # Some programs need SUID wrappers, can be configured further or are
  # started in user sessions.
  # programs.mtr.enable = true;
  # programs.gnupg.agent = {
  #   enable = true;
  #   enableSSHSupport = true;
  #   pinentryFlavor = "gnome3";
  # };
  programs.zsh.ohMyZsh = {
    enable = true;
    theme = "agnoster";
  };

  # List services that you want to enable:

  # Enable the OpenSSH daemon.
  services.openssh.enable = true;

  # Open ports in the firewall.
  # networking.firewall.allowedTCPPorts = [ ... ];
  # networking.firewall.allowedUDPPorts = [ ... ];
  # Or disable the firewall altogether.
  # networking.firewall.enable = false;

  # Enable CUPS to print documents.
  # services.printing.enable = true;

  # Enable sound.
  sound.enable = true;
  hardware.pulseaudio.enable = true;

  # Enable the X11 windowing system.
  services.xserver.enable = true;
  services.xserver.layout = "us";
  # services.xserver.xkbOptions = "eurosign:e";

  # Enable touchpad support.
  #services.xserver.libinput.enable = true;
  services.xserver.libinput = {
    enable = true;
    disableWhileTyping = true;
    naturalScrolling = true;
    scrollMethod = "twofinger";
    tapping = true; 
    tappingDragLock = false;
  };
  # Enable the KDE Desktop Environment.
  services.xserver.desktopManager.plasma5.enable = true;
 
  services.xserver.displayManager = {
    sddm.enable = true;
    sddm.theme = "/sugar-candy";
   };

  # Define a user account. Don't forget to set a password with ‘passwd’.
  users.users.archnix = {
    home = "/home/archnix";
    shell = pkgs.zsh;
    isNormalUser = true;
    extraGroups = [ "wheel" "audio" ]; # Enable ‘sudo’ for the user.
  };

  programs.zsh = {
    enable = true;
    autosuggestions.enable = true;
    enableCompletion= true;
    syntaxHighlighting.enable = true;
  };
  # This value determines the NixOS release with which your system is to be
  # compatible, in order to avoid breaking some software such as database
  # servers. You should change this only after NixOS release notes say you
  # should.
  system.stateVersion = "20.03"; # Did you read the comment?

}

