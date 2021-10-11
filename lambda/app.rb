# frozen_string_literal: true

require 'rack'
require 'serverless_rack'

require './web_app'
require './compiler'

$app = Rack::Builder.new do
  run WebApp
end.to_app

module LambdaFunction
  # Handle a API Gateway/ALB-structured HTTP request using the Sinatra app
  class HttpHandler
    def self.process(event:, context:)
      handle_request(app: $app, event: event, context: context)
    end
  end

  # Handle a non-HTTP proxied request, returning either the compiled result or
  # an error as JSON.
  class DirectHandler
    REVISION = ENV.fetch('REVISION', 'unknown')

    def self.process(event:, context:)
      return { type: 'keep_alive' } if event.has_key?('keep_alive')

      keymap_data = event.fetch('keymap') do
        return error(status: 400, message: 'Missing required argument: keymap')
      end

      keymap_data =
        begin
          Base64.strict_decode64(keymap_data)
        rescue ArgumentError
          return error(status: 400, message: 'Invalid Base64 in keymap input')
        end

      result, log =
        begin
          Compiler.new.compile(keymap_data)
        rescue Compiler::CompileError => e
          return error(status: e.status, message: e.message, detail: e.log)
        end

      result = Base64.strict_encode64(result)

      { type: 'result', result: result, log: log, revision: REVISION }
    rescue StandardError => e
      error(status: 500, message: "Unexpected error: #{e.class}", detail: [e.message])
    end

    def self.error(status:, message:, detail: nil)
      { type: 'error', status: status, message: message, detail: detail, revision: REVISION }
    end
  end
end
