package com.capstone.indoorpositioning.screens;

import com.badlogic.gdx.Gdx;
import com.badlogic.gdx.Screen;
import com.badlogic.gdx.graphics.GL20;
import com.badlogic.gdx.graphics.Texture;
import com.badlogic.gdx.graphics.g2d.Sprite;
import com.badlogic.gdx.maps.tiled.TiledMap;
import com.badlogic.gdx.maps.tiled.TiledMapTileLayer;
import com.badlogic.gdx.maps.tiled.renderers.OrthogonalTiledMapRenderer;
import com.badlogic.gdx.maps.tiled.TmxMapLoader;
import com.badlogic.gdx.graphics.OrthographicCamera;
import com.badlogic.gdx.scenes.scene2d.Stage;
import com.capstone.indoorpositioning.entities.User;
import com.capstone.indoorpositioning.onclick.TiledMapStage;

public class Play implements Screen {

    private TiledMap map;
    private OrthogonalTiledMapRenderer renderer;
    private OrthographicCamera camera;
    private User user;
    private Stage stage;

    @Override
    public void show() {
        LevelManager.loadLevel("LiFi_MobileApp.tmx");
//        map = new TmxMapLoader().load("LiFi_MobileApp.tmx");
        map = LevelManager.map;

        renderer = new OrthogonalTiledMapRenderer(map);

        user = new User(new Sprite(new Texture("user.png")), (TiledMapTileLayer) map.getLayers().get(0));
        user.setPosition(2 * user.getCollisionLayer().getTileWidth(), (user.getCollisionLayer().getHeight() - 7) * user.getCollisionLayer().getTileHeight());

        stage = new TiledMapStage(map, user);
        Gdx.input.setInputProcessor(stage);

        camera = new OrthographicCamera();
        camera.setToOrtho(false, user.getCollisionLayer().getWidth() * user.getCollisionLayer().getTileWidth(),
                user.getCollisionLayer().getHeight() * user.getCollisionLayer().getTileHeight());
    }

    @Override
    public void render(float delta) {
        Gdx.gl.glClearColor(0,0,0,1);
        Gdx.gl.glClear(GL20.GL_COLOR_BUFFER_BIT);

        renderer.setView(camera);
        renderer.render();

        stage.getViewport().setCamera(camera);
        stage.act();

        renderer.getBatch().begin();
        user.draw(renderer.getBatch());
        renderer.getBatch().end();

    }

    @Override
    public void resize(int width, int height) {
        camera.viewportWidth = width;
        camera.viewportHeight = height;
        camera.update();
    }

    @Override
    public void pause() {

    }

    @Override
    public void resume() {

    }

    @Override
    public void hide() {
        dispose();
    }

    @Override
    public void dispose() {
        map.dispose();
        renderer.dispose();
        user.getTexture().dispose();
        stage.dispose();
    }
}
