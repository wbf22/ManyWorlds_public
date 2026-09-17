# ManyWorlds
Manyworlds is open source! However, branding of Manyworlds and the creation of merch associated with the game is reserved for the time being. However, you can make a fork with a new name and new features, and that belongs to you.

## License

[GAME NAME]'s code and assets are open source under the [MIT License](LICENSE) —
you're free to use, modify, and even sell your own fork, commercially or
otherwise.

The name **"[GAME NAME]"** and its logo are trademarks of
[YOUR NAME / STUDIO NAME] and are **not** covered by the open source license.
If you fork the project, please give it your own name and branding rather
than presenting it as "[GAME NAME]." Official merchandise and the
"[GAME NAME]" brand are reserved for the original project.

Have questions about using the name or branding — fan project, collab, or
otherwise? Reach out at [YOUR CONTACT EMAIL].

## Contributing

Contributions are welcome! By submitting a pull request, you agree that
your contribution is provided under the same MIT License as the rest of
the project.

(Optional, add later if desired: a lightweight sign-off / DCO requirement
for contributions, e.g. `git commit -s`, confirming the contributor has
the right to submit the code under this license.)




## Set UP

steps
- Install vscode. 
- Clone the repo
- get a copy of 'Godot_v4.5-stable_linux.x86_64' from Brandon
- get a copy of '.godot' from Brandon. (You might also be able to run the editor to generate this, though this hasn't been tried)
- Then in the vscode debug window run 'Debug Game'

I love how simple godot makes this. We could eventually add the .godot folder and the godot executable to the git repo. They're just a little big.


## Common Problems


### Objects have weird shadows

![image](SCREEN_SHOTS/fbx_problem.png)

This has to do with normals and is a common problem exporting an fbx from blender to ue5. The fix is to choose 'face' when exporting the model in blender under: `Geometry > Smoothing `

(on the right side when exporting to fbx in blender)


### Visual Studio Code is unable to watch for file changes in this large workspace" (error ENOSPC)

For this you need to increase the max file watches on your system. For Ubuntu it's this:
```
sudo vim /etc/sysctl.conf
```

At this to the end of the file `fs.inotify.max_user_watches = 524288`

The run this to load it:
```
sudo sysctl -p
```

## debugging stops working suddenly in godot
running `scons --clean` is all you need



