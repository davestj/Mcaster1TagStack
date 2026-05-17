# config/deploy/production.rb — production stage (ovh-us-east).

set :stage, :production

# Single-host deploy targeting ovh-us-east via the SSH alias in
# ~/.ssh/config. The Capistrano user is the operator's normal SSH user
# (dstjohn) escalated to sudo for systemctl + nginx-free port bind.
server "ovh-us-east", user: "dstjohn", roles: %w[app web db ollama]

set :ssh_options,
    forward_agent: true,
    auth_methods: %w[publickey],
    keepalive: true
