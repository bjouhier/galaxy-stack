{
	'targets': [
		{
			'target_name': 'galaxy_stack',
			'include_dirs': [
				'<@(nodedir)/deps/v8',
			],
			'sources': [
				'src/galaxy-stack.cc',
			],
			'cflags!': ['-ansi'],
		},
	],
}
