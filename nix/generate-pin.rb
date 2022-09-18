#!/usr/bin/env ruby
# frozen_string_literal: true

require 'json'
require 'net/http'
require 'shellwords'

def get_channel_url(channel)
  uri = URI("https://channels.nixos.org/#{channel}")
  res = Net::HTTP.get_response(uri)
  raise 'Not a redirect' unless res.is_a?(Net::HTTPRedirection)
  res['location']
end

def fetch_tarball(url)
  hash = `nix-prefetch-url --unpack #{Shellwords.escape(url)}`
  raise 'Prefetch failed' unless $?.success?
  hash.chomp
end

def generate_pin(channel)
  channel_url = get_channel_url(channel)
  nixexprs = "#{channel_url}/nixexprs.tar.xz"
  hash = fetch_tarball(nixexprs)
  { url: nixexprs, sha256: hash }
end

def main
  channel = ENV.fetch('CHANNEL', 'nixpkgs-unstable')
  pin = generate_pin(channel)
  puts JSON.pretty_generate(pin)
end

main
