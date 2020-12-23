windows:
	python setup.py build_ext --inplace
	pyinstaller --noconfirm --onefile --console "./mp_miner.py"
linux-gnu:
	python3 setup.py build_ext --inplace
	pyinstaller --noconfirm --onefile --console "./mp_miner.py"