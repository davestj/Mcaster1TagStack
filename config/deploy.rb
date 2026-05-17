# config/deploy.rb — shared Capistrano config across all stages.
# See config/deploy/production.rb for stage-specific overrides.

# Capistrano keeps at most this many releases on the server
set :keep_releases, 5

# Project identity
set :application, "tagstack"
set :repo_url,    "git@github.com:davestj/Mcaster1TagStack.git"
set :branch,      "master"

# Where the daemon lives on the server
set :deploy_to,   "/var/www/tagstack.mcaster1.com"

# Files / directories shared across releases (NOT under git, survives deploys)
append :linked_files,
       "daemon/etc/tagstackd.yaml",
       "daemon/etc/ollama.yaml",
       "web/etc/config.php"

append :linked_dirs,
       "shared/log",
       "shared/cache"

# What to deploy — daemon + web subset of the repo
set :scm,          :git
set :format,       :pretty
set :pty,          false

# Hooks
namespace :daemon do
  desc "Build the daemon binary on the release"
  task :build do
    on roles(:app) do
      within release_path do
        execute :bash, "-c", "cd daemon && cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Release && cmake --build build"
      end
    end
  end

  desc "Reload the systemd unit"
  task :restart do
    on roles(:app) do
      execute :sudo, "systemctl reload-or-restart tagstackd"
    end
  end
end

# Apply pending SQL migrations after upload, before symlink swap
namespace :db do
  desc "Apply pending sql/ migrations"
  task :migrate do
    on roles(:app) do
      within release_path do
        execute :bash, "scripts/migrate.sh", "production"
      end
    end
  end
end

# Run Ollama persona Modelfile installs if present
namespace :ollama do
  desc "Install/update personas from daemon/personas/"
  task :build_personas do
    on roles(:app) do
      execute :bash, "-c",
        "for d in #{release_path}/daemon/personas/*/; do " \
        "[ -f \"$d/Modelfile\" ] && ollama create $(basename $d) -f $d/Modelfile; done"
    end
  end
end

after "deploy:updated", "daemon:build"
after "deploy:updated", "db:migrate"
after "deploy:updated", "ollama:build_personas"
after "deploy:published", "daemon:restart"
