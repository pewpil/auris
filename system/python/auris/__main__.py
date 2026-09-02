"""Allow `python -m auris` to invoke the CLI entry point."""

from auris.cli import main

raise SystemExit(main())
