{
	"targets": [
		{
			"target_name": "addon",
			"sources": ["source/addon.cc", "source/mouse.cc"],
			"link_settings": {
				"libraries": [
					"/System/Library/Frameworks/CoreGraphics.framework"
				]
			},
			"include_dirs": [
				"<!(node -e \"require('nan')\")"
			]
		}
	]
}
