package main

import (
	"fmt"
	"github.com/gin-contrib/sessions"
	"github.com/gin-contrib/sessions/cookie"
	"github.com/gin-gonic/gin"
	"net/http"
	"sync"
)

var count = 1

type Task struct {
	context *gin.Context
	flag    bool
	cond    *sync.Cond
	result  ExecResult
}

var taskQueue = make([]*Task, 0)

func runTask(t *Task) {
	type ExecForm struct {
		SQL string `json:"sql" binding:"required"`
	}
	var form ExecForm
	var c = t.context
	c.BindJSON(&form)
	session := sessions.Default(c)

	sessionId := session.Get("count").(int)
	if sessionId == 1 {
		count++
		sessionId = count
		session.Set("count", count)
		session.Save()
	}
	result := Exec(form.SQL, sessionId)
	t.result = result
	t.flag = true
	t.cond.Signal()
}

func newTask(c *gin.Context) *Task {
	return &Task{
		c,
		false,
		sync.NewCond(new(sync.Mutex)),
		ExecResult{},
	}
}

func main() {
	router := gin.Default()
	store := cookie.NewStore([]byte("secret"))
	router.Use(sessions.Sessions("mysession", store))
	const dist = "../frontend/dist"

	cond := sync.NewCond(new(sync.Mutex))

	// static files
	router.Static("/", dist)
	// api
	apiRouter := router.Group("/api")

	apiRouter.POST("/exec", func(c *gin.Context) {
		cond.L.Lock()
		var curTask = newTask(c)
		taskQueue = append(taskQueue, curTask)
		cond.Broadcast()
		cond.L.Unlock()
		curTask.cond.L.Lock()
		for !curTask.flag {
			curTask.cond.Wait()
		}
		curTask.cond.L.Unlock()
		fmt.Println(curTask.result)
		c.JSON(http.StatusOK, curTask.result)
	})

	apiRouter.POST("/info", func(c *gin.Context) {
		result := Info()
		c.JSON(http.StatusOK, result)
	})

	// handle vue history mode
	router.NoRoute(func(c *gin.Context) {
		c.File(dist + "/index.html")
	})

	Setup()

	go func() {
		for {
			cond.L.Lock()
			for len(taskQueue) == 0 {
				cond.Wait()
			}
			task := taskQueue[0]
			taskQueue = taskQueue[1:]
			runTask(task)
			cond.L.Unlock()
		}
	}()

	router.Run()
}
