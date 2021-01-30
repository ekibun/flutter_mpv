import 'package:flutter/material.dart';
import 'package:flutter_mpv/flutter_mpv.dart';

void main() {
  runApp(MyApp());
}

class MyApp extends StatefulWidget {
  @override
  _MyAppState createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  FlutterMpv _texture = FlutterMpv();

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
          appBar: AppBar(
            title: const Text('Plugin example app'),
          ),
          body: Stack(
            children: [
              FutureBuilder(
                future: _texture.getTextureId(),
                builder: (ctx, snapshot) {
                  if (snapshot.data == null) return Container();
                  return Texture(textureId: snapshot.data);
                },
              ),
              FloatingActionButton(onPressed: () {
                _texture.play();
              })
            ],
          )),
    );
  }
}
