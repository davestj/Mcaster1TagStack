# Ollama Personas — Mcaster1TagStack

The TagStack daemon ships against a local Ollama instance with
broadcast-specific personas. Each persona is a custom Modelfile
running on top of a foundation model.


## Installed personas (as of 2026-05-17)

| Persona              | Foundation       | Use case                              |
|----------------------|------------------|---------------------------------------|
| `music-intel`        | command-r        | Metadata enrichment, track context, intel from sparse signals |
| `teleprompter-feed`  | gemma2 / qwen3   | Show-prep content, talkup lines       |
| `podcast-cohost`     | llama3.1         | Episode summaries, segment ideas      |
| `dj-axiom`           | dolphin3         | DJ break copy, station IDs            |


## Foundation models available

`llama3.1:8b`, `qwen3:14b`, `qwen3:8b`, `command-r`, `gemma2:9b`,
`deepseek-r1:8b`, `qwen2.5-coder:14b`, `qwen2.5vl:7b`,
`moondream`, `deepseek-ocr`, `openhermes`, `dolphin3:8b`.

Vision models (`qwen2.5vl`, `moondream`, `deepseek-ocr`) can be
used for album-art analysis and genre/mood inference from cover
imagery.


## Persona Modelfile conventions

Each persona lives at `daemon/personas/<name>/Modelfile`. Build with:

```
ollama create music-intel -f daemon/personas/music-intel/Modelfile
```

Each Modelfile includes:
- `FROM` line — foundation model
- `SYSTEM` block — persona instructions, output format, refusals
- `PARAMETER` overrides (temperature, top_k, etc.)


## Adding a new persona

1. Pick a foundation model that suits the use case (small + fast for
   throughput, large + reasoning for nuance).
2. Write the Modelfile under `daemon/personas/<name>/`.
3. Reference the persona from `daemon/etc/ollama.example.yaml`
   under the relevant feature (`enrichment`, `transcription`, etc.).
4. Add a row to the table at the top of this file.
5. `cap production deploy` — Capistrano runs `ollama create` on
   release as part of the post-deploy hook.


## When NOT to use a persona

For deterministic data extraction (parse this stream-title string into
artist + title) prefer regex / SongDataParser logic over an LLM. Use
personas where ambiguity, taste, or generative copy is the task.


## Output format

Every persona response is JSON-wrapped:

```
{
  "ok": true,
  "persona": "music-intel",
  "model": "command-r",
  "result": <persona-specific payload>,
  "latency_ms": 412
}
```
