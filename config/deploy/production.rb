# config/deploy/production.rb — production stage (ovh-us-east).

set :stage, :production

# Single-host deploy targeting ovh-us-east via the SSH alias in
# ~/.ssh/config. The Capistrano user is the operator's SSH user on
# this box (bthlops), sudo-capable for systemctl + port bind tasks.
server "ovh-us-east", user: "bthlops", roles: %w[app web db ollama]

set :ssh_options,
    forward_agent: true,
    auth_methods: %w[publickey],
    keepalive: true
