# TODO

- [ ] Mesh構築時のASフラグの渡し方を再考する
- [ ] shared_ptrを使う必要があるのか検討する
  - メンバ変数に持たせて後から初期化できること
  - initializer_listをそのまま構築できること
- [ ] shader関連ライブラリ抜きのVulkanSDKに対応する（vcpkg経由に変更）
- [ ] debugCallBackにユーザー側からブレークポイント張れるようにする
- [ ] GitHub Actionsでビルド以外にテストを実行
- [ ] 暗黙メンバ関数を明示的にする
- [ ] camera.processMouseDragLeft()などはInputと切り離した命名にする
