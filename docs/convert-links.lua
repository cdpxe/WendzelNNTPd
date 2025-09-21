function Link(el)
	-- Remove references to Markdown files from link targets, so that the link targets also work inside a PDF.
	-- Replace for example upgrade.md#upgrading with #upgrading.
	el.target = string.gsub(el.target, ".+%.md#", "#")
	return el
end
