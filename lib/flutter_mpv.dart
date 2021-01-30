import 'dart:async';

import 'package:flutter/services.dart';

final MethodChannel _channel = const MethodChannel('flutter_mpv');

class FlutterMpv {
  Future<dynamic> _renderer;
  Future<dynamic> _ensureRenderer() {
    if (_renderer == null) _renderer = _channel.invokeMethod('createTexture');
    return _renderer;
  }

  int _textureId;
  Future<dynamic> getTextureId() async {
    if (_textureId != null) return _textureId;
    final renderer = await _ensureRenderer();
    _textureId = await _channel.invokeMethod('getTextureId', renderer);
    return _textureId;
  }

  play() async {
    final renderer = await _renderer;
    return _channel.invokeMethod('play', renderer);
  }

  close() async {
    if (_renderer == null) return;
    _textureId = null;
    final renderer = await _renderer;
    return _channel.invokeMethod('closeTexture', renderer);
  }
}
