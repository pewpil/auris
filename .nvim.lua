vim.g.neotree_project_opts = {
	filesystem = {
		filtered_items = {
			always_show_by_pattern = { "*/docs/*", "*/thesis/*" },
			never_show = { ".git" },
		},
	},
}

vim.g.telescope_files_allowlisted_dirs = {
	".opencode/",
	{ dir = "docs/*", no_ignore = true },
	{ dir = "thesis/*", no_ignore = true },
}
