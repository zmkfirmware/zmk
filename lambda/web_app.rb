# frozen_string_literal: true

require 'sinatra/base'
require './compiler'

class WebApp < Sinatra::Base
  set :environment, :production
  set :show_exceptions, false
  set :logging, nil
  set :default_content_type, 'application/json'

  def json_body(hash)
    body(hash.to_json)
  end

  post '/api/compile' do
    request.body.rewind
    keymap_data = request.body.read
    result, log = Compiler.new.compile(keymap_data)

    status 200
    content_type 'application/octet-stream'
    headers 'X-Debug-Output': log.to_json
    body result
  end

  error Compiler::CompileError do
    e = env['sinatra.error']
    status e.status
    json_body(error: e.message, detail: e.log)
  end

  error do
    e = env['sinatra.error']
    status 500
    json_body(error: "Unexpected error: #{e.class}", detail: [e.message])
  end

  not_found do
    status 404
    json_body(error: 'No such path', detail: nil)
  end
end
