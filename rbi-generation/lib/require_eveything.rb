class SorbetRBIGeneration::RequireEverything
  # Goes through the most common ways to require all your userland code
  def self.require_everything
    load_rails
    load_bundler # this comes second since some rails projects fail `Bundler.require' before rails is loaded
    require_all_files
  end

  def self.load_rails
    return unless File.exist?('config/application.rb')
    begin
      require 'rails'
    rescue
      return
    end
    require './config/application'
    Rails.application.require_environment!
    Rails.application.eager_load!

    [
      'ActionCable::Connection::Base',
      'ActionController::Base',
      'ActionDispatch::SystemTestCase',
      'ActionMailer::Base',
      'ActionMailer::MessageDelivery',
      'ActiveJob::Base',
      'ActiveRecord::Schema',
      'ActiveRecord::Migration::Current',
    ].each do |const|
      begin
        Object.const_get(const, false)
      rescue
      end
    end
  end

  def self.load_bundler
    begin
      require 'bundler'
    rescue
      return
    end
    Bundler.require
  end

  def self.require_all_files
    files = Dir.glob("**/*.rb")
    errors = []
    files.each do |file|
      begin
        require_relative "#{Dir.pwd}/#{file}"
      rescue LoadError, NoMethodError
        next
      rescue
        errors << file
        next
      end
    end
    # one more chance for order dependent things
    errors.each do |file|
      begin
        require_relative "#{Dir.pwd}/#{file}"
      rescue
      end
    end
  end
end

def at_exit(*args, &block)
  if File.split($0).last == 'rake'
    # Let `rake test` work
    super
    return
  end
  puts "Ignoring at_exit: #{args} #{block}"
end