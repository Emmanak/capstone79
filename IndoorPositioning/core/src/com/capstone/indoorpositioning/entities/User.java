package com.capstone.indoorpositioning.entities;

import com.badlogic.gdx.graphics.g2d.Batch;
import com.badlogic.gdx.Gdx;
import com.badlogic.gdx.graphics.g2d.Sprite;
import com.badlogic.gdx.maps.tiled.TiledMapTileLayer;
import com.badlogic.gdx.maps.tiled.TiledMapTileLayer.Cell;
import com.badlogic.gdx.math.Vector2;
import com.capstone.indoorpositioning.pathfinding.GraphPathImp;

/**
 * Created by JackH_000 on 3/9/17.
 */

public class User extends Sprite {

    /** the movement velocity **/
    private Vector2 velocity = new Vector2();

    private float speed = 60 * 2, gravity = 60 * 1.8f;

    private TiledMapTileLayer collisionLayer;

    private GraphPathImp resultPath;

    public User(Sprite sprite, TiledMapTileLayer collisionLayer){
        super(sprite);
        this.collisionLayer = collisionLayer;
    }

    private String blockedKey = "blocked";

    @Override
    public void draw(Batch batch) {
        update(Gdx.graphics.getDeltaTime());
        super.draw(batch);
    }


    public void update(float delta){
        //apply gravity
        velocity.y -= gravity * delta;

        // clamp velocity
        if(velocity.y > speed )
            velocity.y = speed;
        else if(velocity.y < -speed)
            velocity.y = -speed;

        //save old position
        float oldX = getX(), oldY = getY(), tileWidth = collisionLayer.getTileWidth(), tileHeight = collisionLayer.getTileHeight();
        boolean collisionX = false, collisionY = false;

        setX(getX() + velocity.x * delta);

        if(velocity.x < 0){
            //top left
            collisionX = collisionLayer.getCell((int) (getX()/tileWidth), (int) ((getY() + getHeight())/tileHeight)).
                    getTile().getProperties().containsKey("blocked");

            //middle left
            if(!collisionX)
                collisionX = collisionLayer.getCell((int) (getX()/tileWidth), (int) ((getY() + getHeight()/2)/tileHeight)).
                        getTile().getProperties().containsKey("blocked");

            //bottom left
            if(!collisionX)
                collisionX = collisionLayer.getCell((int) (getX()/tileWidth), (int) (getY()/tileHeight)).
                        getTile().getProperties().containsKey("blocked");

        } else if(velocity.x > 0) {
            //top right
            collisionX = collisionLayer.getCell((int) ((getX() + getWidth())/tileWidth), (int) ((getY() + getHeight())/tileHeight)).
                    getTile().getProperties().containsKey("blocked");

            //middle right
            if(!collisionX)
                collisionX = collisionLayer.getCell((int) ((getX() + getWidth())/tileWidth), (int) ((getY() + getHeight()/2)/tileHeight)).
                        getTile().getProperties().containsKey("blocked");

            //bottom right
            if(!collisionX)
                collisionX = collisionLayer.getCell((int) ((getX() + getWidth())/tileWidth), (int) (getY()/tileHeight)).
                        getTile().getProperties().containsKey("blocked");
        }

        //react to x collision
        if(collisionX){
            setX(oldX);
            velocity.x = 0;
        }

        //move on y
        setY(getY() + velocity.y * delta);

        if(velocity.y < 0){
            //bottom left
            collisionY = collisionLayer.getCell((int)(getX()/tileWidth), (int)(getY()/tileHeight)).
                    getTile().getProperties().containsKey("blocked");

            //bottom middle
            if(!collisionY)
                collisionY = collisionLayer.getCell((int)((getX() + getWidth()/2)/tileWidth), (int)(getY()/tileHeight)).
                        getTile().getProperties().containsKey("blocked");

            //bottom right;
            if(!collisionY)
                collisionY = collisionLayer.getCell((int)((getX() + getWidth())/tileWidth), (int)(getY()/tileHeight)).
                        getTile().getProperties().containsKey("blocked");

        } else if(velocity.y > 0){
            //top left
            collisionY = collisionLayer.getCell((int)(getX()/tileWidth), (int)((getY()+ getHeight())/tileHeight)).
                    getTile().getProperties().containsKey("blocked");
            //top middle
            if(!collisionY)
                collisionY = collisionLayer.getCell((int)((getX() + getWidth()/2)/tileWidth), (int)((getY()+ getHeight())/tileHeight)).
                        getTile().getProperties().containsKey("blocked");
            //top right
            if(!collisionY)
                collisionY = collisionLayer.getCell((int)(getX() + getWidth()/tileWidth), (int)((getY()+ getHeight())/tileHeight)).
                        getTile().getProperties().containsKey("blocked");

        }

        //react to y collision
        if(collisionY) {
            setY(oldY);
            velocity.y = 0;
        }

    }

    private boolean isCellBlocked(float x, float y) {
        Cell cell = collisionLayer.getCell((int) (x / collisionLayer.getTileWidth()), (int) (y / collisionLayer.getTileHeight()));
        return cell != null && cell.getTile() != null && cell.getTile().getProperties().containsKey(blockedKey);
    }

    public Vector2 getVelocity() {
        return velocity;
    }

    public void setVelocity(Vector2 velocity) {
        this.velocity = velocity;
    }

    public float getSpeed() {
        return speed;
    }

    public void setSpeed(float speed) {
        this.speed = speed;
    }

    public float getGravity() {
        return gravity;
    }

    public void setGravity(float gravity) {
        this.gravity = gravity;
    }

    public TiledMapTileLayer getCollisionLayer() {
        return collisionLayer;
    }

    public void setCollisionLayer(TiledMapTileLayer collisionLayer) {
        this.collisionLayer = collisionLayer;
    }
    public void setResultPath(GraphPathImp resultPath) {
        this.resultPath = resultPath;
    }
}
